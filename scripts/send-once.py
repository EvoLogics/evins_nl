#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLData, NLDataReport, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

def report_listener(data):
    rospy.loginfo("Report\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/data", NLData, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/data", NLData, listener, queue_size=1)
        sub_report = rospy.Subscriber("/evins_nl_1/data_report", NLDataReport, report_listener, queue_size=1)
        
        msg = NLData()

        msg.command.id = NLCommand.SEND
        msg.command.status = NLCommand.UNDEFINED
        msg.command.subject = NLCommand.DATA
        msg.data = "test"
        msg.route.destination = 5
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
