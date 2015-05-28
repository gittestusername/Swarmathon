/*
 * Author: Karl A. Stolleis
 * Maintainer: Karl A. Stolleis
 * Email: karl.a.stolleis@nasa.gov; kurt.leucht@nasa.gov
 * NASA Center: Kennedy Space Center
 * Mail Stop: NE-C1
 * 
 * Project Name: Swarmie Robotics NASA Center Innovation Fund
 * Principal Investigator: Cheryle Mako
 * Email: cheryle.l.mako@nasa.gov
 * 
 * Date Created: June 6, 2014
 * Safety Critical: NO
 * NASA Software Classification: D
 * 
 * This software is copyright the National Aeronautics and Space Administration (NASA)
 * and is distributed under the GNU LGPL license.  All rights reserved.
 * Permission to use, copy, modify and distribute this software is granted under
 * the LGPL and there is no implied warranty for this software.  This software is provided
 * "as is" and NASA or the authors are not responsible for indirect or direct damage
 * to any user of the software.  The authors and NASA are under no obligation to provide
 * maintenence, updates, support or modifications to the software.
 * 
 * Revision Log:
 *      
 */

#include <string.h>
#include <unistd.h>  
#include <sstream>
#include <vector>

#include <ros/ros.h>
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/Pose.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/NavSatStatus.h>
#include <tf/transform_datatypes.h>
#include <tf/tf.h>

using namespace std;

void imuHandler(const sensor_msgs::Imu::ConstPtr& message);
void gpsHandler(const sensor_msgs::NavSatFix::ConstPtr& message);
//void publishData(const ros::TimerEvent& e);
//void publishData(void);

char host[128];

//geometry_msgs::Pose2D pose2d;
//geometry_msgs::Pose pose;
geometry_msgs::Pose2D realRobotLocation;
geometry_msgs::Pose2D rawRobotLocation;  
geometry_msgs::Pose2D gazeboRobotLocation;  
geometry_msgs::Pose2D unmRobotLocation;  
geometry_msgs::Pose2D imageRobotLocation;  
geometry_msgs::Pose2D mapRobotLocation;  

geometry_msgs::Pose2D robotOrigin;
bool originHasBeenSet = false;

ros::Subscriber imuSubscriber;
ros::Subscriber gpsSubscriber;
//ros::Publisher pose2dPublish;
//ros::Publisher posePublish;
ros::Publisher originPublish;
// KWL added a series of location topics, each to be used where appropriate
// Now no other code needs to do any coordinate system translations or rotations.  It's all done here.
ros::Publisher locRealPublish; 
ros::Publisher locRawPublish;
ros::Publisher locGazeboPublish;
ros::Publisher locUnmPublish;
ros::Publisher locImagePublish;
ros::Publisher locMapPublish;

ros::Timer publishTimer;

int main(int argc, char **argv) {

    gethostname(host, sizeof (host));
    string hostname(host);
    string publishedName;

    if (argc >= 2) {
        publishedName = argv[1];
        cout << "Welcome to the world of tomorrow " << publishedName << "!  Localization module started." << endl;
    } else {
        publishedName = hostname;
        cout << "No Name Selected. Default is: " << publishedName << endl;
    }

    ros::init(argc, argv, (publishedName + "_LOCALIZATION"));
    ros::NodeHandle lNH;
    
    imuSubscriber = lNH.subscribe((publishedName + "/imu"), 10, imuHandler);
    gpsSubscriber = lNH.subscribe((publishedName + "/gps"), 10, gpsHandler);

    //pose2dPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/pose2d"), 10);
    //posePublish = lNH.advertise<geometry_msgs::Pose>((publishedName + "/pose"), 10);

    originPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/origin"), 10);

    locRealPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_real"), 10);
    locRawPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_raw"), 10);
    locGazeboPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_gazebo"), 10);
    locUnmPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_unm"), 10);
    locImagePublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_image"), 10);
    locMapPublish = lNH.advertise<geometry_msgs::Pose2D>((publishedName + "/location_map"), 10);

    //publishTimer = lNH.createTimer(ros::Duration(0.20), publishData);

    //while (ros::ok()) {
        ros::spin();
    //}

    return EXIT_SUCCESS;
}

void imuHandler(const sensor_msgs::Imu::ConstPtr& message) {
    //pose.orientation = message->orientation;
    // This is due to the quaternion conversion and IMU data in sim
    // actual robot may need compass values negated to correspond correctly
    // with sim values.
    double yaw = (tf::getYaw(message->orientation) * 180.0 / M_PI);
    if (yaw < 0) {
        //pose2d.theta = 360 - (360 + temp);
        realRobotLocation.theta = 360 - (360 + yaw);
    } else {
        //pose2d.theta = 360 - temp;
        realRobotLocation.theta = 360 - yaw;
    }
    
    // calculate all other thetas relative to this one
    rawRobotLocation.theta = realRobotLocation.theta;
    gazeboRobotLocation.theta = realRobotLocation.theta;
    unmRobotLocation.theta = realRobotLocation.theta;
    imageRobotLocation.theta = realRobotLocation.theta;
    mapRobotLocation.theta = realRobotLocation.theta;

    // normalize so it doesn't go above 360 or below 0
// TODO: do it.  Should be a subroutine in mobility you can use here
}

void gpsHandler(const sensor_msgs::NavSatFix::ConstPtr& message) {
    double tempx = message->longitude;
    double tempy = message->latitude;
    if ( (message->status.status != sensor_msgs::NavSatStatus::STATUS_NO_FIX) && 
		(originHasBeenSet == false) && (tempx != 0) && (tempy != 0) ) {
        robotOrigin.x = tempx;
        robotOrigin.y = tempy;
        originHasBeenSet = true;
    } else if (originHasBeenSet == false) {
        robotOrigin.x = 0;
        robotOrigin.y = 0;
    }

    //pose2d.x = pose.position.x = tempx;
    //pose2d.y = pose.position.y = tempy;

    unmRobotLocation.x = ((tempx - robotOrigin.x) * 100000);
    unmRobotLocation.y = ((tempy - robotOrigin.y) * 111180);  // this is a magic number because 1 deg of lat is not the same linear
                                                     // distance everywhere - this is the starting point for 28 deg N (KSC)

    // calculate all other x,y values relative to this one
    rawRobotLocation.x = tempx; // in degrees
    gazeboRobotLocation.x = unmRobotLocation.y; // in meters
    realRobotLocation.x = unmRobotLocation.y; // in meters
    imageRobotLocation.x = roundf( (unmRobotLocation.y + 15.0) * 10 ); // in pixels, not meters
    mapRobotLocation.x = roundf(unmRobotLocation.y * 10); // in pixels, not meters
    rawRobotLocation.y = tempy; // in degrees
    gazeboRobotLocation.y = -1 * unmRobotLocation.x; // in meters
    realRobotLocation.y = -1 * unmRobotLocation.x; // in meters
    imageRobotLocation.y = roundf( (unmRobotLocation.x + 15.0) * 10 ); // in pixels, not meters
    mapRobotLocation.y = roundf(unmRobotLocation.x * 10); // in pixels, not meters

    // all location data is ready to be published
    //publishData();
    //pose2dPublish.publish(pose2d);
    //posePublish.publish(pose);
    originPublish.publish(robotOrigin);
    locRealPublish.publish(realRobotLocation);
    locRawPublish.publish(rawRobotLocation);
    locGazeboPublish.publish(gazeboRobotLocation);
    locUnmPublish.publish(unmRobotLocation);
    locImagePublish.publish(imageRobotLocation);
    locMapPublish.publish(mapRobotLocation);


}

//void publishData(const ros::TimerEvent& e) {
/* void publishData(void) {
    pose2dPublish.publish(pose2d);
    //posePublish.publish(pose);
    originPublish.publish(robotOrigin);
    locRealPublish.publish(realRobotLocation);
    locRawPublish.publish(rawRobotLocation);
    locGazeboPublish.publish(gazeboRobotLocation);
    locUnmPublish.publish(unmRobotLocation);
    locImagePublish.publish(imageRobotLocation);
    locMapPublish.publish(mapRobotLocation);
} */




