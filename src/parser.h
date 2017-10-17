/* Copyright (c) 2017, Oleksiy Kebkal <lesha@evologics.de>
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. The name of the author may not be used to endorse or promote products 
 *    derived from this software without specific prior written permission. 
 * 
 * Alternatively, this software may be distributed under the terms of the 
 * GNU General Public License ("GPL") version 2 as published by the Free 
 * Software Foundation. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#ifndef EVINS_NL_PARSER_H
#define EVINS_NL_PARSER_H

#include <math.h> 

#include <iostream>
#include <sstream>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

#include <ros/ros.h>

#include "evins_nl/NLRaw.h"
#include "evins_nl/NLVersion.h"
#include "evins_nl/NLAddress.h"
#include "evins_nl/NLProtocol.h"
#include "evins_nl/NLProtocols.h"
#include "evins_nl/NLProtocolInfo.h"
#include "evins_nl/NLRouting.h"
#include "evins_nl/NLNeighbours.h"
#include "evins_nl/NLStates.h"
#include "evins_nl/NLStatistics.h"
#include "evins_nl/NLData.h"
#include "evins_nl/NLDataReport.h"
#include "evins_nl/NLDiscovery.h"
#include "evins_nl/NLPolling.h"

#include "diagnostic_msgs/KeyValue.h"

#include "comm_middlemen.h"
#include "config.h"
#include "std_msgs/Bool.h"

using boost::asio::ip::tcp;
using evins_nl::NLRaw;
using evins_nl::NLVersion;
using evins_nl::NLAddress;
using evins_nl::NLProtocol;
using evins_nl::NLProtocols;
using evins_nl::NLProtocolInfo;
using evins_nl::NLRouting;
using evins_nl::NLNeighbours;
using evins_nl::NLStates;
using evins_nl::NLStatistics;
using evins_nl::NLData;
using evins_nl::NLDataReport;
using evins_nl::NLDiscovery;
using evins_nl::NLPolling;
using std_msgs::Bool;

namespace evins_nl
{

typedef enum { WAITSYNC_NO = 0,
               WAITSYNC_SINGLELINE = 1,
               WAITSYNC_MULTILINE = 2,
               WAITSYNC_BINARY = 3} evins_nl_waitsync_status;
    
class parser
{
  public:
  parser(boost::asio::io_service& io_service, evins_nl::config &config, evins_nl::comm_middlemen *comm)
        :  
        config_(config),
        io_service_(io_service),
        answer_timer_(io_service),
        waitsync_(WAITSYNC_NO),
        eol_("\r\n")
    {
        pub_connection_ = nh_.advertise<Bool>(config.nodeName() + "/connection", 100);
        
        pub_raw_ = nh_.advertise<NLRaw>(config.nodeName() + "/raw", 100);

        pub_version_ = nh_.advertise<NLVersion>(config.nodeName() + "/version", 100);
        sub_version_ = nh_.subscribe(config.nodeName() + "/version", 100, &parser::versionCallback, this);

        pub_address_ = nh_.advertise<NLAddress>(config.nodeName() + "/address", 100);
        sub_address_ = nh_.subscribe(config.nodeName() + "/address", 100, &parser::addressCallback, this);

        pub_protocol_ = nh_.advertise<NLProtocol>(config.nodeName() + "/protocol", 100);
        sub_protocol_ = nh_.subscribe(config.nodeName() + "/protocol", 100, &parser::protocolCallback, this);

        pub_protocols_ = nh_.advertise<NLProtocols>(config.nodeName() + "/protocols", 100);
        sub_protocols_ = nh_.subscribe(config.nodeName() + "/protocols", 100, &parser::protocolsCallback, this);

        pub_protocolinfo_ = nh_.advertise<NLProtocolInfo>(config.nodeName() + "/protocolinfo", 100);
        sub_protocolinfo_ = nh_.subscribe(config.nodeName() + "/protocolinfo", 100, &parser::protocolinfoCallback, this);

        pub_routing_ = nh_.advertise<NLRouting>(config.nodeName() + "/routing", 100);
        sub_routing_ = nh_.subscribe(config.nodeName() + "/routing", 100, &parser::routingCallback, this);

        pub_neighbours_ = nh_.advertise<NLNeighbours>(config.nodeName() + "/neighbours", 100);
        sub_neighbours_ = nh_.subscribe(config.nodeName() + "/neighbours", 100, &parser::neighboursCallback, this);

        pub_states_ = nh_.advertise<NLStates>(config.nodeName() + "/states", 100);
        sub_states_ = nh_.subscribe(config.nodeName() + "/states", 100, &parser::statesCallback, this);

        pub_statistics_ = nh_.advertise<NLStatistics>(config.nodeName() + "/statistics", 100);
        sub_statistics_ = nh_.subscribe(config.nodeName() + "/statistics", 100, &parser::statisticsCallback, this);

        pub_data_ = nh_.advertise<NLData>(config.nodeName() + "/data", 100);
        sub_data_ = nh_.subscribe(config.nodeName() + "/data", 100, &parser::dataCallback, this);
        pub_data_report_ = nh_.advertise<NLDataReport>(config.nodeName() + "/data_report", 100);

        pub_discovery_ = nh_.advertise<NLDiscovery>(config.nodeName() + "/discovery", 100);
        sub_discovery_ = nh_.subscribe(config.nodeName() + "/discovery", 100, &parser::discoveryCallback, this);
        
        pub_polling_ = nh_.advertise<NLPolling>(config.nodeName() + "/polling", 100);
        sub_polling_ = nh_.subscribe(config.nodeName() + "/polling", 100, &parser::pollingCallback, this);
        
        comm_ = comm;

        version_.command.id = version_.command.status = NLCommand::UNDEFINED;
        version_.command.subject = NLCommand::VERSION;

        address_.command.id = address_.command.status = NLCommand::UNDEFINED;
        address_.command.subject = NLCommand::ADDRESS;
        address_.address = 0;
        
        protocol_.command.id = protocol_.command.status = NLCommand::UNDEFINED;
        protocol_.command.subject = NLCommand::PROTOCOL;

        protocols_.command.id = protocols_.command.status = NLCommand::UNDEFINED;
        protocols_.command.subject = NLCommand::PROTOCOLS;

        protocolinfo_.command.id = protocolinfo_.command.status = NLCommand::UNDEFINED;
        protocolinfo_.command.subject = NLCommand::PROTOCOLINFO;

        routing_.command.id = routing_.command.status = NLCommand::UNDEFINED;
        routing_.command.subject = NLCommand::ROUTING;

        neighbours_.command.id = neighbours_.command.status = NLCommand::UNDEFINED;
        neighbours_.command.subject = NLCommand::NEIGHBOURS;

        states_.command.id = states_.command.status = NLCommand::UNDEFINED;
        states_.command.subject = NLCommand::STATES;

        statistics_.command.id = statistics_.command.status = NLCommand::UNDEFINED;
        statistics_.command.subject = NLCommand::STATISTICS;

        data_.command.id = data_.command.status = NLCommand::UNDEFINED;
        data_.command.subject = NLCommand::DATA;

        discovery_.command.id = discovery_.command.status = NLCommand::UNDEFINED;
        discovery_.command.subject = NLCommand::DISCOVERY;

        polling_.command.id = polling_.command.status = NLCommand::UNDEFINED;
        polling_.command.subject = NLCommand::POLLING;
    }

    void connected(void)
    {
        Bool conn;
        conn.data = true;
        pub_connection_.publish(conn);
        ROS_INFO_STREAM("connect");
    }

    void disconnected(void)
    {
        Bool conn;
        conn.data = false;
        pub_connection_.publish(conn);
        ROS_INFO_STREAM("disconnect");
        waitsync_ = WAITSYNC_NO;
    }

    /* async headers: NL,routing NL,recv
     */
    
    void to_term(std::vector<uint8_t> chunk, std::size_t len)
    {
        std::string schunk;
        std::vector<uint8_t>::iterator it = chunk.begin();
        for (int cnt = 0; cnt < len; it++, cnt++) {
            schunk.insert(schunk.end(), *it);
        }
        ROS_INFO_STREAM("Parsing new data: " << schunk);
        ROS_INFO_STREAM("waitsync_ : " << waitsync_);
        ROS_INFO_STREAM("more_: " << more_);
        more_ += schunk;
        publishRaw(schunk);
        
        size_t before, after;
        do {
            before = more_.length();
            to_term_nl();
            after = more_.length();
        } while (after > 0 && after != before);
    }

    void versionCallback(const evins_nl::NLVersion::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        version_.header.stamp = msg->header.stamp;
        version_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,version" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }
    
    void addressCallback(const evins_nl::NLAddress::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        address_.header.stamp = msg->header.stamp;
        address_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::SET) {
            stream << "NL,set,address," << int(msg->address) <<  eol_;
        } else if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,address" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }

    void protocolCallback(const evins_nl::NLProtocol::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        protocol_.header.stamp = msg->header.stamp;
        protocol_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::SET) {
            stream << "NL,set,protocol," << msg->protocol << eol_;
        } else if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,protocol" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }

    void protocolsCallback(const evins_nl::NLProtocols::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        protocols_.header.stamp = msg->header.stamp;
        protocols_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,protocols" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }

    void protocolinfoCallback(const evins_nl::NLProtocolInfo::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        protocolinfo_.header.stamp = msg->header.stamp;
        protocolinfo_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,protocolinfo," << msg->protocol << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }

    void routingCallback(const evins_nl::NLRouting::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        routing_.header.stamp = msg->header.stamp;
        routing_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,routing" << eol_;
        } else if (msg->command.id == NLCommand::SET) {
            stream << "NL,set,routing";
            std::vector<NLRoute>::const_iterator it;
            if (msg->routing.size() == 0) {
                ROS_ERROR_STREAM("" << ": cannot set empty routing");
                return;
            }
            for(it = msg->routing.begin(); it != msg->routing.end(); ++it) {
                if ((*it).source == 0) {
                    stream << ",default->" << (*it).destination;
                } else {
                    stream << "," << (*it).source << "->" << (*it).destination;
                }
            }
            stream << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void neighboursCallback(const evins_nl::NLNeighbours::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        neighbours_.header.stamp = msg->header.stamp;
        neighbours_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,neighbours" << eol_;
        } else if (msg->command.id == NLCommand::DELETE) {
            stream << "NL,delete,neighbour,";
            stream << int(msg->neighbours[0].address);
            stream << eol_;
        } else if (msg->command.id == NLCommand::SET) {
            stream << "NL,set,neighbours";
            std::vector<NLNeighbour>::const_iterator it;
            if (msg->neighbours.size() == 0) {
                ROS_ERROR_STREAM("" << ": cannot set empty neighbours");
                return;
            }
            for(it = msg->neighbours.begin(); it != msg->neighbours.end(); ++it) {
                if (msg->type == NLNeighbours::SHORT) {
                    stream << "," << int((*it).address);
                } else {
                    stream << "," << int((*it).address)
                           << ":" << int((*it).rssi)
                           << ":" << int((*it).integrity)
                           << ":" << int((*it).time);
                }
            }
            stream << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void statesCallback(const evins_nl::NLStates::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        states_.header.stamp = msg->header.stamp;
        states_.command.id = msg->command.id;
        states_.command.subject = msg->command.subject;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET && msg->command.subject == NLCommand::STATE) {
            stream << "NL,get,state" << eol_;
        } else if (msg->command.id == NLCommand::GET && msg->command.subject == NLCommand::STATES) {
            stream << "NL,get,states" << eol_;
        } else if (msg->command.id == NLCommand::RESET && msg->command.subject == NLCommand::STATE) {
            stream << "NL,reset,state" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command id " << int(msg->command.id)
                << " : " << int(msg->command.subject));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void statisticsCallback(const evins_nl::NLStatistics::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        statistics_.header.stamp = msg->header.stamp;
        statistics_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET && msg->type == NLStatistics::NEIGHBOURS) {
            stream << "NL,get,statistics,neighbours" << eol_;
        } else if (msg->command.id == NLCommand::GET && msg->type == NLStatistics::PATHS) {
            stream << "NL,get,statistics,paths" << eol_;
        } else if (msg->command.id == NLCommand::GET && msg->type == NLStatistics::DATA) {
            stream << "NL,get,statistics,data" << eol_;
        } else if (msg->command.id == NLCommand::CLEAR && msg->type == NLStatistics::DATA) {
            stream << "NL,clear,statistics,data" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command / type: " << int(msg->command.id)
                << " : " << int(msg->type));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void dataCallback(const evins_nl::NLData::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        data_.header.stamp = msg->header.stamp;
        data_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::SEND) {
            int len = msg->data.length();
            stream << "NL,send,";
            switch (msg->datatype) {
            case NLData::TOLERANT:
                stream << "tolerant,";
                break;
            case NLData::SENSITIVE:
                stream << "sensitive,";
                break;
            case NLData::BROADCAST:
                stream << "broadcast,";
                break;
            case NLData::ALARM:
                stream << "alarm,";
                break;
            }
            stream << len << "," << int(msg->route.destination) << "," << msg->data << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command: " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void discoveryCallback(const evins_nl::NLDiscovery::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        discovery_.header.stamp = msg->header.stamp;
        discovery_.command.id = msg->command.id;

        discovery_.period = msg->period;
        discovery_.total = msg->total;
        
        std::ostringstream stream;
        if (msg->command.id == NLCommand::GET) {
            stream << "NL,get,discovery" << eol_;
        } else if (msg->command.id == NLCommand::START) {
            stream << "NL,start,discovery," << int(msg->period) << "," << int(msg->total) <<eol_;
        } else if (msg->command.id == NLCommand::STOP) {
            stream << "NL,stop,discovery" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command: " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    

    void pollingCallback(const evins_nl::NLPolling::ConstPtr& msg)
    {
        if (msg->command.status != NLCommand::UNDEFINED) {
            /* ignore published by oursleves message */
            return;
        }
        polling_.header.stamp = msg->header.stamp;
        polling_.command.id = msg->command.id;

        std::ostringstream stream;
        if (msg->command.id == NLCommand::SET) {
            stream << "NL,set,polling";
            polling_.sequence = msg->sequence;
            for (int i = 0; i < msg->sequence.size(); i++) {
                stream << "," << int(msg->sequence[i]);
            }
            stream << eol_;
        } else if (msg->command.id == NLCommand::START) {
            polling_.flag = msg->flag;
            std::string flag;
            if (msg->flag == NLPolling::BURST) { flag = "b"; } else { flag = "nb";}
            stream << "NL,start,polling," << flag <<eol_;
        } else if (msg->command.id == NLCommand::STOP) {
            stream << "NL,stop,polling" << eol_;
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": unsupported command: " << int(msg->command.id));
            return;
        }
        std::string telegram = stream.str();
        sendSync(telegram);
    }    
    
    void handle_answer_timeout(const boost::system::error_code& error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
        ROS_ERROR_STREAM("Answer timeout: waitsync: " << waitsync_);
        waitsync_ == WAITSYNC_NO;
        /* publishSync("ERROR ANSWER TIMEOUT", WAITSYNC_NO); */
    }
    
  private:
    boost::asio::io_service& io_service_;
    evins_nl::comm_middlemen *comm_;
    evins_nl::config config_;
    /* parser state */
    ros::NodeHandle nh_;
    evins_nl_waitsync_status waitsync_;
    std::string request_;
    std::string request_parameters_;
    std::string eol_;
    std::string more_;
    boost::asio::deadline_timer answer_timer_;

    ros::Publisher pub_connection_;
    
    ros::Publisher pub_raw_;

    ros::Publisher pub_version_;
    ros::Subscriber sub_version_;
    evins_nl::NLVersion version_;
    
    ros::Publisher pub_address_;
    ros::Subscriber sub_address_;
    evins_nl::NLAddress address_;

    ros::Publisher pub_protocol_;
    ros::Subscriber sub_protocol_;
    evins_nl::NLProtocol protocol_;

    ros::Publisher pub_protocols_;
    ros::Subscriber sub_protocols_;
    evins_nl::NLProtocols protocols_;

    ros::Publisher pub_protocolinfo_;
    ros::Subscriber sub_protocolinfo_;
    evins_nl::NLProtocolInfo protocolinfo_;

    ros::Publisher pub_routing_;
    ros::Subscriber sub_routing_;
    evins_nl::NLRouting routing_;

    ros::Publisher pub_neighbours_;
    ros::Subscriber sub_neighbours_;
    evins_nl::NLNeighbours neighbours_;

    ros::Publisher pub_states_;
    ros::Subscriber sub_states_;
    evins_nl::NLStates states_;

    ros::Publisher pub_statistics_;
    ros::Subscriber sub_statistics_;
    evins_nl::NLStatistics statistics_;

    ros::Publisher pub_data_;
    ros::Subscriber sub_data_;
    evins_nl::NLData data_;

    ros::Publisher pub_data_report_;
    
    ros::Publisher pub_discovery_;
    ros::Subscriber sub_discovery_;
    evins_nl::NLDiscovery discovery_;

    ros::Publisher pub_polling_;
    ros::Subscriber sub_polling_;
    evins_nl::NLPolling polling_;
    
    void sendSync(std::string &message)
    {
        if (waitsync_ == WAITSYNC_NO) {
            comm_->send(message);
            publishRaw(message);
            
            answer_timer_.cancel();
            answer_timer_.expires_from_now(boost::posix_time::milliseconds(1000));
            answer_timer_.async_wait(boost::bind(&parser::handle_answer_timeout, this,
                                                 boost::asio::placeholders::error));
        } else {
            ROS_ERROR_STREAM("" << __func__ << ": sequence error: " << message);
        }
    }
    
    void publishRaw(std::string raw)
    {
        NLRaw raw_msg;
        raw_msg.stamp = ros::Time::now();
        raw_msg.command = raw;

        pub_raw_.publish(raw_msg);
    }
    
    void to_term_nl()
    {
        /* need NL,send,broadcast message type to embed it into instant/piggyback message in the interrogation loop
         * depending on its role */
        static const boost::regex eol_regex("\r*\n");
        if (boost::regex_search(more_, eol_regex))
        {
            static const boost::regex rcv_regex("^NL,recv,(.*)");
            boost::smatch rcv_matches;
            if (boost::regex_match(more_, rcv_matches, rcv_regex)) {
                nl_rcv_extract(rcv_matches[1].str());
            } else {
                static const boost::regex multiline_regex(
                    "^(NL,(states|statistics|protocolinfo),)(.*?)");
                boost::smatch multiline_matches;
                if (boost::regex_match(more_, multiline_matches, multiline_regex)) {
                    std::string rest = multiline_matches[3].str();
                    static const boost::regex multiline_params_regex("^((.*)\r*\n\r*\n)(.*)");
                    boost::smatch multiline_params_matches;
                    if (boost::regex_match(rest, multiline_params_matches, multiline_params_regex)) {
                        std::string parameters = multiline_params_matches[2].str();
                        int len = parameters.length();
                        if (parameters[len-1] == '\r') {
                            parameters.erase(len-1,len);
                        }
                        nl_extract_subject(multiline_matches[2].str(), parameters);
                        more_.erase(0, multiline_matches[1].str().length());
                        more_.erase(0, multiline_params_matches[1].str().length());
                    } else {
                        ROS_WARN_STREAM("need more data: " << more_.data());
                    }
                } else {
                    static const boost::regex short_regex("^(NL,([^,]*?)\r*\n)(.*)");
                    boost::smatch short_matches;
                    if (boost::regex_match(more_, short_matches, short_regex)) {
                        std::string report = short_matches[2].str();
                        waitsync_ == WAITSYNC_NO;
                        if (report == "error") {
                            ROS_ERROR_STREAM("" << __func__ << ": unexpected error report");
                        } else {
                            ROS_ERROR_STREAM("" << __func__ << ": unexpected report: " << report);
                        }
                        more_.erase(0, short_matches[1].str().length());
                    } else {
                        static const boost::regex general_regex("^(NL,([^,]*),*(.*?)\n).*");
                        boost::smatch general_matches;
                        if (boost::regex_match(more_, general_matches, general_regex)) {
                            std::string parameters = general_matches[3].str();
                            int len = parameters.length();
                            if (parameters[len-1] == '\r') {
                                parameters.erase(len-1,len);
                            }
                            nl_extract_subject(general_matches[2].str(), parameters);
                            more_.erase(0, general_matches[1].str().length());
                        } else {
                            ROS_WARN_STREAM("need more data: " << more_.data());
                        }
                    }
                }
            }
        } else {
            ROS_WARN_STREAM("need more data: " << more_.data());
        }
    }

    void nl_rcv_extract(std::string tail)
    {
       static const boost::regex recv_regex("^(\\d+),(\\d+),(\\d+),(.*)");
       boost::smatch recv_matches;

       data_.command.id = NLCommand::RECV;
       data_.command.subject = NLCommand::DATA;
       data_.command.status = NLCommand::OK;
       
       if (boost::regex_match(tail, recv_matches, recv_regex))
       { /* format matched, check length */
           int len = boost::lexical_cast<int>(recv_matches[1].str());
           data_.route.source = boost::lexical_cast<int>(recv_matches[2].str());
           data_.route.destination = boost::lexical_cast<int>(recv_matches[3].str());

           boost::regex recv_payload_regex("^(.{" + boost::lexical_cast<std::string>(len) + "})\r*\n(.*)");
           boost::smatch recv_payload_matches;
           std::string rest = recv_matches[4].str();
           if (boost::regex_match(rest, recv_payload_matches, recv_payload_regex)) {
               data_.data = recv_payload_matches[1].str();
               more_ = recv_payload_matches[2].str();
               pub_data_.publish(data_);
           } else {
               if (len + 2 >= rest.length()) {
                   ROS_ERROR_STREAM("Cannot extract payload of length: " 
                                    << len << ": in " << rest);
                   more_.erase(0, more_.find("\n") + 1);
               } else {
                   /* need more data */;
               }
           }
       } else {
           static const boost::regex lf("\n");
           if (boost::regex_search(tail, lf)) {
               ROS_WARN_STREAM("RECV parse error: " << tail);
               more_.erase(0, more_.find("\n") + 1);
           } else {
               /* need more data */
           }
       }
    }

    void nl_extract_subject(std::string subject, std::string parameters)
    {
        if (subject == "version") {
            nl_extract_version(parameters);
        } else if (subject == "address") {
            nl_extract_address(parameters);
        } else if (subject == "protocol") {
            nl_extract_protocol(parameters);
        } else if (subject == "protocols") {
            nl_extract_protocols(parameters);
        } else if (subject == "protocolinfo") {
            nl_extract_protocolinfo(parameters);
        } else if (subject == "routing") {
            nl_extract_routing(parameters);
        } else if (subject == "neighbours") {
            nl_extract_neighbours(parameters);
        } else if (subject == "neighbour") {
            nl_extract_neighbour(parameters);
        } else if (subject == "state") {
            nl_extract_state(parameters);
        } else if (subject == "states") {
            nl_extract_states(parameters);
        } else if (subject == "statistics") {
            nl_extract_statistics(parameters);
        } else if (subject == "send") {
            nl_extract_send(parameters);
        } else if (subject == "delivered") {
            nl_extract_report(subject, parameters);
        } else if (subject == "failed") {
            nl_extract_report(subject, parameters);
        } else if (subject == "discovery") {
            nl_extract_discovery(parameters);
        } else if (subject == "polling") {
            nl_extract_polling(parameters);
        } else {
            ROS_WARN_STREAM("TODO: subject: " << subject << " parameters: " << parameters);
        }
    }

    void nl_extract_version(std::string parameters)
    {
        answer_timer_.cancel();
        version_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            version_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex version_regex("(.*),(.*),(.*)");
            boost::smatch version_matches;
            if (boost::regex_match(parameters, version_matches, version_regex)) {
                version_.vmajor = boost::lexical_cast<int>(version_matches[1].str());
                version_.vminor = boost::lexical_cast<int>(version_matches[2].str());
                version_.description = version_matches[3].str();
                version_.command.status = NLCommand::OK;
            } else {
                ROS_ERROR_STREAM("" << ": version parsing error: " << parameters);
                version_.command.status = NLCommand::ERROR;
            }
        }
        pub_version_.publish(version_);
    }

    void nl_extract_address(std::string parameters)
    {
        answer_timer_.cancel();
        address_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            address_.command.status = NLCommand::ERROR;
        } else {
            int value = boost::lexical_cast<int>(parameters);
            
            address_.address = value;
            address_.command.status = NLCommand::OK;
        }
        pub_address_.publish(address_);
    }

    void nl_extract_protocol(std::string parameters)
    {
        answer_timer_.cancel();
        protocol_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            protocol_.command.status = NLCommand::ERROR;
        } else {
            protocol_.protocol = parameters;
            protocol_.command.status = NLCommand::OK;
        }
        pub_protocol_.publish(protocol_);
    }

    void nl_extract_protocols(std::string parameters)
    {
        answer_timer_.cancel();
        protocols_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            protocols_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);

            while (l.size() > 0) {
                protocols_.protocols.push_back(*(l.begin()));
                l.pop_front();
            }
            protocols_.command.status = NLCommand::OK;
        }
        pub_protocols_.publish(protocols_);
    }
    
    void nl_extract_protocolinfo(std::string parameters)
    {
        answer_timer_.cancel();
        protocolinfo_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            protocolinfo_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",*\r*\n");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);

            protocolinfo_.map.erase(protocolinfo_.map.begin(),protocolinfo_.map.end());
            /* first parameter: "Protocol", rest: "Key : Value" */
            protocolinfo_.protocol = *(l.begin());
            l.pop_front();
            while (l.size() > 0) {
                diagnostic_msgs::KeyValue kv;
                std::string kvstring = *(l.begin());
                static const boost::regex kv_regex("(.*) : (.*)");
                boost::smatch kv_matches;
                if (boost::regex_match(kvstring, kv_matches, kv_regex)) {
                    kv.key = kv_matches[1].str();
                    kv.value = kv_matches[2].str();
                    protocolinfo_.map.push_back(kv);
                }
                l.pop_front();
            }
            protocolinfo_.command.status = NLCommand::OK;
        }
        pub_protocolinfo_.publish(protocolinfo_);
    }

    void nl_extract_routing(std::string parameters)
    {
        answer_timer_.cancel();
        routing_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            routing_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);

            routing_.routing.erase(routing_.routing.begin(),routing_.routing.end());
            while (l.size() > 0) {
                std::string route = *(l.begin());
                static const boost::regex route_regex("(.*)->(.*)");
                boost::smatch route_matches;
                if (boost::regex_match(route, route_matches, route_regex)) {
                    NLRoute route;
                    if (route_matches[1].str() == "default") {
                        route.source = 0;
                    } else {
                        route.source = boost::lexical_cast<int>(route_matches[1].str());
                    }
                    route.destination = boost::lexical_cast<int>(route_matches[2].str());
                    routing_.routing.push_back(route);
                }
                l.pop_front();
            }
            routing_.command.status = NLCommand::OK;
        }
        pub_routing_.publish(routing_);
    }
    
    void nl_extract_neighbours(std::string parameters)
    {
        answer_timer_.cancel();
        neighbours_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            neighbours_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",");
            static const boost::regex dots(":");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);

            neighbours_.neighbours.erase(neighbours_.neighbours.begin(),neighbours_.neighbours.end());
            if (boost::regex_search(more_, dots)) {
                neighbours_.type = NLNeighbours::LONG;
            } else {
                neighbours_.type = NLNeighbours::SHORT;
            }
            neighbours_.command.status = NLCommand::OK;
            while (l.size() > 0) {
                std::string neighbour_str = *(l.begin());
                NLNeighbour neighbour;
                if (neighbours_.type == NLNeighbours::SHORT) {
                    neighbour.address = boost::lexical_cast<int>(neighbour_str);
                } else {
                    static const boost::regex neighbour_regex("(.*):(.*):(.*):(.*)");
                    boost::smatch neighbour_matches;
                    if (boost::regex_match(neighbour_str, neighbour_matches, neighbour_regex)) {
                        neighbour.address = boost::lexical_cast<int>(neighbour_matches[1].str());
                        neighbour.rssi = boost::lexical_cast<int>(neighbour_matches[2].str());
                        neighbour.integrity = boost::lexical_cast<int>(neighbour_matches[3].str());
                        neighbour.time = boost::lexical_cast<int>(neighbour_matches[4].str());
                    } else {
                        ROS_ERROR_STREAM("" << ": neighbours parsing error: " << neighbour_str);
                        neighbours_.neighbours.erase(neighbours_.neighbours.begin(),neighbours_.neighbours.end());
                        neighbours_.command.status = NLCommand::ERROR;
                        break;
                    }
                }
                neighbours_.neighbours.push_back(neighbour);
                l.pop_front();
            }
        }
        pub_neighbours_.publish(neighbours_);
    }

    void nl_extract_neighbour(std::string parameters)
    {
        answer_timer_.cancel();
        neighbours_.header.stamp = ros::Time::now();

        if (parameters == "error") {
            neighbours_.command.status = NLCommand::ERROR;
        } else if (parameters == "ok") {
            neighbours_.neighbours.erase(neighbours_.neighbours.begin(),neighbours_.neighbours.end());
            neighbours_.command.status = NLCommand::OK;
        } else {
            ROS_ERROR_STREAM("" << ": unsupported neighbour status" << parameters);
            neighbours_.command.status = NLCommand::ERROR;
        }
        pub_neighbours_.publish(neighbours_);
    }

    void nl_extract_state(std::string parameters)
    {
        answer_timer_.cancel();
        states_.header.stamp = ros::Time::now();

        states_.command.subject = NLCommand::STATE;
        if (parameters == "error") {
            states_.command.status = NLCommand::ERROR;
        } else if (parameters == "ok") {
            states_.states.erase(states_.states.begin(),states_.states.end());
            states_.command.status = NLCommand::OK;
        } else {
            static const boost::regex state_regex("([^(]*).([^)]*).");
            boost::smatch state_matches;
            states_.states.erase(states_.states.begin(),states_.states.end());
            if (boost::regex_match(parameters, state_matches, state_regex)) {
                NLState state;
                state.state = state_matches[1].str();
                state.event = state_matches[2].str();
                states_.states.push_back(state);
                states_.command.status = NLCommand::OK;
            } else {
                ROS_ERROR_STREAM("" << ": states parsing error: " << parameters);
                states_.command.status = NLCommand::ERROR;
            }
        }
        pub_states_.publish(states_);
    }

    void nl_extract_states(std::string parameters)
    {
        answer_timer_.cancel();
        states_.header.stamp = ros::Time::now();

        states_.command.subject = NLCommand::STATES;
        if (parameters == "error") {
            states_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",*\r*\n");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);

            states_.states.erase(states_.states.begin(),states_.states.end());
            while (l.size() > 0) {
                NLState state;
                std::string state_str = *(l.begin());
                static const boost::regex state_regex("([^(]*).([^)]*).");
                boost::smatch state_matches;
                if (boost::regex_match(state_str, state_matches, state_regex)) {
                    state.state = state_matches[1].str();
                    state.event = state_matches[2].str();
                    states_.states.push_back(state);
                }
                l.pop_front();
            }
            states_.command.status = NLCommand::OK;
        }
        pub_states_.publish(states_);
    }
 
    void nl_extract_statistics(std::string parameters)
    {
        answer_timer_.cancel();
        statistics_.header.stamp = ros::Time::now();

        statistics_.statistics.erase(statistics_.statistics.begin(),statistics_.statistics.end());
        if (parameters == "error") {
            statistics_.command.status = NLCommand::ERROR;
        } else if (parameters == "ok") {
            statistics_.command.status = NLCommand::OK;
        } else {
            static const boost::regex lf(",*\r*\n");
            static const boost::regex comma(",");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, lf);

            /* first parameter: type, rest: statistics */
            std::string type_str = *(l.begin());
            
            l.pop_front();
            statistics_.command.status = NLCommand::OK;

            if (boost::regex_search(type_str, comma)) {
                std::list<std::string> lp;
                boost::regex_split(std::back_inserter(lp), type_str, comma);
                type_str = *(lp.begin());
                lp.pop_front();
                std::string status = *(lp.begin());
                ROS_ERROR_STREAM("" << ": status: " << status);
                if (status == "empty") {
                    statistics_.command.status = NLCommand::EMPTY;
                } else {
                    statistics_.command.status = NLCommand::ERROR;
                }
            }
            if (type_str == "neighbours") {
                statistics_.type = NLStatistics::NEIGHBOURS;
            } else if (type_str == "paths") {
                statistics_.type = NLStatistics::PATHS;
            } else if (type_str == "data") {
                statistics_.type = NLStatistics::DATA;
            } else {
                ROS_ERROR_STREAM("" << ": parsing of unknown type : " << type_str);
                statistics_.command.status = NLCommand::ERROR;
            }
            if (statistics_.command.status == NLCommand::OK) {
                while (l.size() > 0) {
                    NLStatisticsItem item;
                    std::string stats = *(l.begin());
                    static boost::regex stats_regex;
                    switch (statistics_.type) {
                    case NLStatistics::NEIGHBOURS:
                        stats_regex = boost::regex(" (.*) neighbour:(\\d+) count:(\\d+) total:(\\d+)");
                        break;
                    case NLStatistics::PATHS:
                         /* {Role,Path,Duration,Count,Total} */
                        stats_regex = boost::regex(" (.*) path:([^ ]*) duration:([^ ]*) count:(\\d+) total:(\\d+)");
                        break;
                    case NLStatistics::DATA:
                        /* {Role,Hash,Len,Duration,State,Total,Dst,Hops} */
                        stats_regex = boost::regex(" (.*) data:0x([^ ]*) len:(\\d+) duration:([^ ]*) state:(.*) total:(\\d+) dst:(\\d+) hops:(\\d+)");
                        break;
                    }
                    
                    ROS_ERROR_STREAM("type: " << __LINE__ << ": " << int(statistics_.type));
                    boost::smatch stats_matches;
                    if (boost::regex_match(stats, stats_matches, stats_regex)) {
                        item.role = stats_matches[1].str();
                        switch (statistics_.type) {
                        case (NLStatistics::NEIGHBOURS): {
                            item.neighbours = boost::lexical_cast<unsigned int>(stats_matches[2].str());
                            item.count = boost::lexical_cast<unsigned int>(stats_matches[3].str());
                            item.total = boost::lexical_cast<unsigned int>(stats_matches[4].str());
                            break;
                        }
                        case (NLStatistics::PATHS): {
                            std::string path = stats_matches[2].str();
                            static const boost::regex comma(",");
                            std::list<std::string> lp;
                            boost::regex_split(std::back_inserter(lp), path, comma);
                            while (lp.size() > 0) {
                                item.path.push_back(boost::lexical_cast<unsigned int>(*(lp.begin())));
                                lp.pop_front();
                            }
                            item.duration = boost::lexical_cast<float>(stats_matches[3].str());
                            item.count = boost::lexical_cast<unsigned int>(stats_matches[4].str());
                            item.total = boost::lexical_cast<unsigned int>(stats_matches[5].str());
                            break;
                        }
                        case (NLStatistics::DATA): {
                            std::stringstream ss;
                            ss << std::hex << stats_matches[2].str();
                            ss >> item.hash;
                            item.len = boost::lexical_cast<unsigned int>(stats_matches[3].str());
                            item.duration = boost::lexical_cast<float>(stats_matches[4].str());
                            item.state = stats_matches[5].str();
                            item.total = boost::lexical_cast<unsigned int>(stats_matches[6].str());
                            item.dst = boost::lexical_cast<unsigned int>(stats_matches[7].str());
                            item.hops = boost::lexical_cast<unsigned int>(stats_matches[8].str());
                            break;
                        }
                        }
                        statistics_.statistics.push_back(item);
                    } else {
                        ROS_ERROR_STREAM("" << ": parsing error : " << stats);
                        statistics_.command.status = NLCommand::ERROR;
                        break;
                    }
                    l.pop_front();
                }
            }
        }
        pub_statistics_.publish(statistics_);
    }

    void nl_extract_send(std::string parameters)
    {
        answer_timer_.cancel();
        data_.header.stamp = ros::Time::now();

        data_.command.subject = NLCommand::SEND;
        if (parameters == "error") {
            data_.command.status = NLCommand::ERROR;
        } else if (parameters == "ok") {
            data_.command.status = NLCommand::OK;
        } else {
            data_.reference = boost::lexical_cast<unsigned int>(parameters);
            data_.command.status = NLCommand::OK;
            // ROS_ERROR_STREAM("" << ": unsupported parameters: " << parameters);
            // data_.command.status = NLCommand::ERROR;
        }
        pub_data_.publish(data_);
    }

    void nl_extract_report(std::string report, std::string parameters)
    {
        answer_timer_.cancel();
        NLDataReport data_report;
        
        data_report.header.stamp = ros::Time::now();

        if (report == "delivered") {
            data_report.report = NLDataReport::DELIVERED;
        } else if (report == "failed") {
            data_report.report = NLDataReport::FAILED;
        } else {
            ROS_ERROR_STREAM("" << ": unsupported report: " << report);
            return;
        }
        static const boost::regex route_regex("(.*),(.*),(.*)");
        boost::smatch route_matches;
        if (boost::regex_match(parameters, route_matches, route_regex)) {
            data_report.reference = boost::lexical_cast<int>(route_matches[1].str());
            data_report.route.source = boost::lexical_cast<int>(route_matches[2].str());
            data_report.route.destination = boost::lexical_cast<int>(route_matches[3].str());
        }
        pub_data_report_.publish(data_report);
    }

    void nl_extract_discovery(std::string parameters)
    {
        answer_timer_.cancel();
        discovery_.header.stamp = ros::Time::now();
        discovery_.command.status = NLCommand::OK;

        if (parameters == "ok") {
            ;
        } else {
            static const boost::regex discovery_regex("(\\d+),(\\d+)");
            boost::smatch discovery_matches;
            if (boost::regex_match(parameters, discovery_matches, discovery_regex)) {
                discovery_.period = boost::lexical_cast<int>(discovery_matches[1].str());
                discovery_.total = boost::lexical_cast<int>(discovery_matches[2].str());
            } else {
                ROS_ERROR_STREAM("" << ": parsing error : " << parameters);
                discovery_.command.status = NLCommand::ERROR;
            }
        }
        pub_discovery_.publish(discovery_);
    }

    void nl_extract_polling(std::string parameters)
    {
        answer_timer_.cancel();
        polling_.header.stamp = ros::Time::now();
        polling_.command.status = NLCommand::OK;

        polling_.sequence.erase(polling_.sequence.begin(),polling_.sequence.end());
        if (parameters == "ok") {
            ;
        } else if (parameters == "error") {
            polling_.command.status = NLCommand::ERROR;
        } else {
            static const boost::regex comma(",");
            std::list<std::string> l;
            boost::regex_split(std::back_inserter(l), parameters, comma);
            while (l.size() > 0) {
                polling_.sequence.push_back(boost::lexical_cast<unsigned int>(*(l.begin())));
                l.pop_front();
            }
        }
        pub_polling_.publish(polling_);
    }
    
};

}  // namespace

#endif  // EVINS_NL_PARSER_H
