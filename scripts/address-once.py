#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLAddress, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/address", NLAddress, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/address", NLAddress, listener, queue_size=1)
        
        msg = NLAddress()

        # msg.address = 2
        # msg.command.id = NLCommand.SET
        
        msg.command.id = NLCommand.GET
        
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
