#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLNeighbours, NLNeighbour, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/neighbours", NLNeighbours, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/neighbours", NLNeighbours, listener, queue_size=1)
        
        msg = NLNeighbours()

        # msg.command.id = NLCommand.SET
        # # msg.type = NLNeighbours.LONG;
        # msg.type = NLNeighbours.SHORT;
        # msg.neighbours.append(NLNeighbour(2,-30,120,10000));
        # msg.neighbours.append(NLNeighbour(3,-35,130,12000));

        # msg.command.id = NLCommand.GET

        msg.command.id = NLCommand.DELETE
        msg.neighbours.append(NLNeighbour(2,0,0,0));
        
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
