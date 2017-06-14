#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLData, NLDataReport, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/data", NLData, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/data", NLData, listener, queue_size=1)
        
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
