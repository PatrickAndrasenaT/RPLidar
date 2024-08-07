#include <ros/ros.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/Imu.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>

// Global variables to store sensor data
sensor_msgs::Imu current_imu;
geometry_msgs::PoseStamped current_pose;

void imuCallback(const sensor_msgs::Imu::ConstPtr& msg) {
    current_imu = *msg;
}

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr& msg) {
    current_pose = *msg;
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "data_fusion_node");
    ros::NodeHandle nh;

    // Subscribers
    ros::Subscriber imu_sub = nh.subscribe("/mavros/imu/data", 10, imuCallback);
    ros::Subscriber pose_sub = nh.subscribe("/slam_out_pose", 10, poseCallback); // Assuming Hector SLAM publishes to /slam_out_pose

    // Publisher
    ros::Publisher fused_pose_pub = nh.advertise<geometry_msgs::PoseStamped>("fused_pose", 10);

    tf::TransformBroadcaster br;
    tf::Transform transform;

    ros::Rate rate(10.0);
    while (nh.ok()) {
        ros::spinOnce(); // Check for new messages

        geometry_msgs::PoseStamped fused_pose;
        fused_pose.header.stamp = ros::Time::now();
        fused_pose.header.frame_id = "map";

        // Use IMU data for orientation
        fused_pose.pose.orientation = current_imu.orientation;

        // Use pose from SLAM for position
        fused_pose.pose.position = current_pose.pose.position;

        // Publish fused pose
        fused_pose_pub.publish(fused_pose);

        // Broadcast the transform
        transform.setOrigin(tf::Vector3(fused_pose.pose.position.x, fused_pose.pose.position.y, fused_pose.pose.position.z));
        tf::Quaternion q(fused_pose.pose.orientation.x, fused_pose.pose.orientation.y, fused_pose.pose.orientation.z, fused_pose.pose.orientation.w);
        transform.setRotation(q);
        br.sendTransform(tf::StampedTransform(transform, ros::Time::now(), "map", "base_link"));

        rate.sleep();
    }

    return 0;
}
