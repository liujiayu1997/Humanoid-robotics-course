#include "robotControler.h"

void robotControler::connectRobot()
{
    if (robot_status == true)
        return;
    else
    {
        if (ConnectDobot(0, 115200, 0, 0, 0) != DobotConnect_NoError) {
            cout << "connect fail" << endl;
            return;
        }
        cout << "connect success" << endl;
        robot_status = true;
        initRobot();
    }
}

void robotControler::disconnectRobot()
{
    robot_status = false;
    DisconnectDobot();
}

void robotControler::movePoint(uint8_t ptpMode, float x, float y, float z)
{
    PTPCmd ptpCmd;

    ptpCmd.ptpMode = ptpMode;
    ptpCmd.x = x;
    ptpCmd.y = y;
    ptpCmd.z = z;
    ptpCmd.r = 0;

    // Send the command. If failed, just resend the command
    uint64_t queuedCmdIndex;
    uint64_t executedCmdIndex;

    SetPTPCmd(&ptpCmd, true, &queuedCmdIndex);
    do {
        GetQueuedCmdCurrentIndex(&executedCmdIndex);
    } while (executedCmdIndex < queuedCmdIndex);
}

void robotControler::getPoint(float& x, float& y, float& z)
{
    Pose pose;
    if (GetPose(&pose) != DobotCommunicate_NoError) {
        cout << "Fail to get pose" << endl;
    }
    x = pose.x;
    y = pose.y;
    z = pose.z;

}

void robotControler::openAirsource(bool status)
{
    uint64_t queuedCmdIndex;
    uint64_t executedCmdIndex;

    // get air source status
    bool isCtrlEnabled, isSucked;
    GetEndEffectorGripper(&isCtrlEnabled, &isSucked);
    int open_status;
    if (SetEndEffectorGripper(true, status, 1, &queuedCmdIndex) != DobotCommunicate_NoError)
        cout << "open air source error" << endl;
    do {
        GetQueuedCmdCurrentIndex(&executedCmdIndex);
    } while (executedCmdIndex < queuedCmdIndex);
    air_source_status = true;
}

void robotControler::closeAirsource()
{
    uint64_t queuedCmdIndex;
    uint64_t executedCmdIndex;

    // get air source status
    bool isCtrlEnabled, isSucked;
    GetEndEffectorGripper(&isCtrlEnabled, &isSucked);
    int open_status;
    if (SetEndEffectorGripper(false, false, 1, &queuedCmdIndex) != DobotCommunicate_NoError)
        cout << "open air source error" << endl;
    do {
        GetQueuedCmdCurrentIndex(&executedCmdIndex);
    } while (executedCmdIndex < queuedCmdIndex);
    air_source_status = false;
}

void robotControler::initRobot()
{
    // Command timeout
    SetCmdTimeout(3000);
    // Clear old commands and set the queued command running
    SetQueuedCmdClear();
    SetQueuedCmdStartExec();

    // Device SN
    char deviceSN[64];
    GetDeviceSN(deviceSN, sizeof(deviceSN));
    cout << "Device SN:" << deviceSN << endl;

    // Device Name
    char deviceName[64];
    GetDeviceName(deviceName, sizeof(deviceName));
    cout << "Device Name:" << deviceName << endl;

    // Device version information
    uint8_t majorVersion, minorVersion, revision, hwVersion;
    GetDeviceVersion(&majorVersion, &minorVersion, &revision, &hwVersion);
    cout << "Device information:V" << int(majorVersion) << '.' << int(minorVersion) << '.' << int(revision) << endl;

    // Set the end effector parameters
    EndEffectorParams endEffectorParams;
    memset(&endEffectorParams, 0, sizeof(EndEffectorParams));
    endEffectorParams.xBias = 71.6f;
    SetEndEffectorParams(&endEffectorParams, false, NULL);

    // 1. Set the JOG parameters
    JOGJointParams jogJointParams;
    for (uint32_t i = 0; i < 4; i++) {
        jogJointParams.velocity[i] = 200;
        jogJointParams.acceleration[i] = 200;
    }
    SetJOGJointParams(&jogJointParams, false, NULL);

    JOGCoordinateParams jogCoordinateParams;
    for (uint32_t i = 0; i < 4; i++) {
        jogCoordinateParams.velocity[i] = 200;
        jogCoordinateParams.acceleration[i] = 200;
    }
    SetJOGCoordinateParams(&jogCoordinateParams, false, NULL);

    JOGCommonParams jogCommonParams;
    jogCommonParams.velocityRatio = 50;
    jogCommonParams.accelerationRatio = 50;
    SetJOGCommonParams(&jogCommonParams, false, NULL);

    // 2. Set the PTP parameters
    PTPJointParams ptpJointParams;
    for (uint32_t i = 0; i < 4; i++) {
        ptpJointParams.velocity[i] = 200;
        ptpJointParams.acceleration[i] = 200;
    }
    SetPTPJointParams(&ptpJointParams, false, NULL);

    PTPCoordinateParams ptpCoordinateParams;
    ptpCoordinateParams.xyzVelocity = 200;
    ptpCoordinateParams.xyzAcceleration = 200;
    ptpCoordinateParams.rVelocity = 200;
    ptpCoordinateParams.rAcceleration = 200;
    SetPTPCoordinateParams(&ptpCoordinateParams, false, NULL);

    PTPJumpParams ptpJumpParams;
    ptpJumpParams.jumpHeight = 10;
    ptpJumpParams.zLimit = 150;
    SetPTPJumpParams(&ptpJumpParams, false, NULL);

    SetQueuedCmdStartExec();
}
