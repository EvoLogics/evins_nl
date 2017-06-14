#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLStatistics, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/statistics", NLStatistics, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/statistics", NLStatistics, listener, queue_size=1)
        
        msg = NLStatistics()

        #msg.command.id = NLCommand.GET
        msg.command.id = NLCommand.CLEAR
        msg.command.status = NLCommand.UNDEFINED
        # msg.type = NLStatistics.NEIGHBOURS
        # msg.type = NLStatistics.PATHS
        msg.type = NLStatistics.DATA
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
