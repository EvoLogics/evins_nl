#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLStates, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/states", NLStates, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/states", NLStates, listener, queue_size=1)
        
        msg = NLStates()

        # msg.command.id = NLCommand.GET
        msg.command.id = NLCommand.RESET
        msg.command.subject = NLCommand.STATE
        # msg.command.subject = NLCommand.STATES
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
