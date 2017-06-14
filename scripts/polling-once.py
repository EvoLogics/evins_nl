#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLPolling, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/polling", NLPolling, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/polling", NLPolling, listener, queue_size=1)
        
        msg = NLPolling()

        # msg.command.id = NLCommand.SET
        # msg.command.id = NLCommand.START
        msg.command.id = NLCommand.STOP
        msg.command.subject = NLCommand.POLLING
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        msg.flag = NLPolling.NO_BURST
        msg.sequence = [2,3]
        
        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
