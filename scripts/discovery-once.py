#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLDiscovery, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/discovery", NLDiscovery, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/discovery", NLDiscovery, listener, queue_size=1)
        
        msg = NLDiscovery()

        # msg.command.id = NLCommand.GET
        # msg.command.id = NLCommand.START
        msg.command.id = NLCommand.STOP
        msg.command.subject = NLCommand.DISCOVERY
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        msg.period = 5
        msg.total = 20
        
        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
