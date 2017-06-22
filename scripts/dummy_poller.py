#!/usr/bin/env python
import rospy
from evins_nl.msg import NLData, NLPolling, NLProtocol, NLDataReport, NLCommand

class STATE(object):
    IDLE = 0
    PROTOCOL = 1
    SEQINI = 2
    SEQRUN = 3
    def __setattr__(self, *_):
        pass

class EVENT(object):
    EPS = 0
    RUN = 1
    TIMEOUT = 2
    ACCEPTED = 3
    REJECTED = 4
    def __setattr__(self, *_):
        pass

class SM(object): 
    state = STATE.IDLE
    event = EVENT.EPS
    waiting_answer = 0
    answer_timer = None
    pub_polling = None

    def wait_answer(self):
        self.waiting_answer = 1
        self.answer_timeout = rospy.Timer(rospy.Duration(1), answer_timeout, True)

    def answer_recv(self):
        self.waiting_answer = 0
        if self.answer_timer != None:
            self.answer_timer.shutdown()
            self.answer_timer = None

sm = SM()
    
def setting_listener(data):
    global sm
    if data.command.status != NLCommand.UNDEFINED:
        sm.answer_recv()
        if data.command.status == NLCommand.OK:
            sm.event = EVENT.ACCEPTED
        else:
            sm.event = EVENT.EJECTED
        handle_event(0)

def recv_listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("RECV: \n%s", data)
        
def answer_timeout(event_):
    global sm
    if sm.waiting_answer == 0:
        pass
    else:
        sm.event = EVENT.TIMEOUT
        handle_event(0)

def set_polling():
    global sm
    msg = NLPolling()
    msg.command.id = NLCommand.SET
    msg.command.subject = NLCommand.POLLING
    msg.command.status = NLCommand.UNDEFINED
    msg.header.stamp = rospy.Time.now()
    msg.sequence = [2,3]
    rospy.loginfo(msg)
    sm.pub_polling.publish(msg)
    sm.wait_answer()

def start_polling():
    global sm
    msg = NLPolling()
    msg.command.id = NLCommand.START
    msg.command.subject = NLCommand.POLLING
    msg.command.status = NLCommand.UNDEFINED
    msg.header.stamp = rospy.Time.now()
    msg.flag = NLPolling.BURST
    rospy.loginfo(msg)
    sm.pub_polling.publish(msg)
    sm.wait_answer()
    
def set_protocol(protocol):
    msg = NLProtocol()
    msg.protocol = protocol
    msg.command.id = NLCommand.SET
    msg.command.status = NLCommand.UNDEFINED
    msg.header.stamp = rospy.Time.now()
    rospy.loginfo(msg)
    sm.pub_protocol.publish(msg)
    sm.wait_answer()    
    
def handle_event(event_):
    global sm
    rospy.loginfo("STATE: %d\n",sm.state)
    switcher = {
        STATE.IDLE : handle_idle,
        STATE.PROTOCOL: handle_protocol,
        STATE.SEQINI: handle_seqini,
        STATE.SEQRUN: handle_seqrun,
    }
    handler = switcher.get(sm.state, lambda: "nothing")
    new_event = handler()
    if (new_event != EVENT.EPS):
        rospy.Timer(rospy.Duration(0.1), handle_event, True)

def handle_idle():
    global sm
    if (sm.event == EVENT.ACCEPTED):
        sm.event = EVENT.RUN    
        sm.state = STATE.PROTOCOL
    else:
        sm.event = EVENT.EPS    
        set_protocol("polling")
    return sm.event

def handle_protocol():
    global sm
    if (sm.event == EVENT.ACCEPTED):
        sm.event = EVENT.RUN    
        sm.state = STATE.SEQINI
    else:
        sm.event = EVENT.EPS    
        set_polling()
    return sm.event

def handle_seqini():
    global sm
    if (sm.event == EVENT.ACCEPTED):
        sm.event = EVENT.RUN    
        sm.state = STATE.SEQRUN
    else:
        sm.event = EVENT.EPS    
        start_polling()
    return sm.event

def handle_seqrun():
    rospy.loginfo("DONE\n")
    return EVENT.EPS

#############

if __name__ == '__main__':
    try:
        rospy.init_node('dummy_poller')
        rate = rospy.Rate(10)
        
        sm.pub_polling = rospy.Publisher("/evins_nl_1/polling", NLPolling, tcp_nodelay=True, queue_size=1)
        sm.sub_polling = rospy.Subscriber("/evins_nl_1/polling", NLPolling, setting_listener, queue_size=1)

        sm.pub_protocol = rospy.Publisher("/evins_nl_1/protocol", NLProtocol, tcp_nodelay=True, queue_size=2)
        sm.sub_protocol = rospy.Subscriber("/evins_nl_1/protocol", NLProtocol, setting_listener, queue_size=1)

        sm.sub_data = rospy.Subscriber("/evins_nl_1/data", NLData, recv_listener, queue_size=1)
        
        rospy.Timer(rospy.Duration(1), handle_event, True)
        rospy.spin()
    except rospy.ROSInterruptException:
        pass
