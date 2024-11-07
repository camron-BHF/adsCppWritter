
#include "AdsLib.h"
#include "AdsVariable.h"
#include "AdsNotificationOOI.h"
#include <array>
#include <vector>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <chrono>

const int numServos = 30;
const int maxObjects = 301;

struct stServo {
    float x, y, radius;
};

struct stObject {
    float x, y, width, height;
};

bool isSeperatedByHyperPlane (const stServo& servo, const stObject& object) {
    float closestX = std::max(object.x, std::min(servo.x, object.x + object.width));
    float closestY = std::max(object.y, std::min(servo.y, object.y + object.height));
    float dx = servo.x - closestX;
    float dy = servo.y - closestY;
    float distanceSquared = dx * dx + dy * dy;
    return distanceSquared > servo.radius * servo.radius;
}

void checkCollisions(const std::array<stServo, numServos>& servos, const std::array<stObject, maxObjects>&allObjects,
                    bool collisionArray[])
{
    float circleSpacing = 5.0f;
    for (const auto& object : allObjects){
        float rectMinX = object.x - circleSpacing;
        float rectMaxX = object.x + object.width + circleSpacing;
        int startIndex = std::max(0, static_cast<int>(std::floor(rectMinX / circleSpacing)));
        int endIndex = std::min(static_cast<int>(servos.size()), static_cast<int>(std::ceil(rectMaxX / circleSpacing)));
        
        for (int i = startIndex; i < endIndex; ++i) {
            if (!collisionArray[i] && !isSeperatedByHyperPlane(servos[i], object)) {
                collisionArray[i] = true;
            }
        }
    }
}

void readServoData(const AdsDevice& route, std::array<stServo, numServos>& servoArray) {
    AdsVariable<std::array<stServo, numServos>> servoVar(route, "MAIN.servoArray");
    servoArray = servoVar;
}

void readObjectData(const AdsDevice& route, std::array<stObject, maxObjects>& objArray) {
    AdsVariable<std::array<stObject, maxObjects>> objVar(route, "MAIN.objArray");
    objArray = objVar;
}

static void readWriteCollisionArray(const AdsDevice& route, const bool collisionData[]) {
    std::array<bool, numServos> collisionArray;

    // Write collision data
    for (size_t i = 0; i < numServos; ++i) {
        collisionArray[i] = collisionData[i];
    }
    AdsVariable<std::array<bool, numServos>> collisionVar{route, "MAIN.collisionArray"};
    collisionVar = collisionArray;

    // Read back collision data for verification
    std::array<bool, numServos> readArray = collisionVar;
    std::cout << "Collision Array Written: ";
    for (const auto& value : collisionArray) {
        std::cout << value << " ";
    }
    std::cout << "\nCollision Array Read Back: ";
    for (const auto& value : readArray) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

void performCollisionDetection(const AdsDevice& route) {
    std::array<stServo, numServos> servoArray;
    std::array<stObject, maxObjects> objArray;
    bool collisionArray[numServos] = {false};

    readServoData(route, servoArray);
    readObjectData(route, objArray);

    auto start = std::chrono::high_resolution_clock::now();
    checkCollisions(servoArray, objArray, collisionArray);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Execution time: " << duration << " microseconds" << std::endl;

    readWriteCollisionArray(route, collisionArray);
}

static void NotifyCallback( const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser){
    (void)pAddr;
    bool trigger;
    std::memcpy(&trigger, pNotification + 1, std::min<size_t>(sizeof(trigger), pNotification->cbSampleSize));

    if(trigger) {
        std::cout << "Trigger recevied, performing collision detection ..." <<std::endl;
        const AdsDevice* route = reinterpret_cast<const AdsDevice*>(static_cast<uintptr_t>(hUser));
        performCollisionDetection(*route);

        //reset the trigger variable to false
        AdsVariable<bool> triggerVar(*route, "MAIN.bTriggerCppCode");
        triggerVar = false;
    }
}

void setupNotification(const AdsDevice& route){
    AdsVariable<bool> triggerVar(route, "MAIN.bTriggerCppCode");

    AdsNotificationAttrib attrib = {
        sizeof(bool),
        ADSTRANS_SERVERONCHA,
        0, 
        {4000000}
    };

    AdsNotification notification {
        route, 
        "MAIN.bTriggerCppCode",
        attrib,
        reinterpret_cast<PAdsNotificationFuncExConst>(&NotifyCallback), //        &NotifyCallback,
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&route))
    };

    std::cout <<"Notification setup complete. Awaiting changed ... \n";
}


int main() {
    try{
        static const AmsNetId remoteNetId { 172, 17, 176, 1, 1, 1};
        static const char remoteIpV4[] = "10.0.0.113";
        AdsDevice route {remoteIpV4, remoteNetId, AMSPORT_R0_PLC_TC3};

        setupNotification(route);
        std::cout<<"Waiting for notifications, press Enter to exit" << std::endl;
        std::cin.get();

    } catch (const AdsException& ex) {
        std::cout <<"Error: " <<ex.errorCode << "\n";
        std::cout <<"AdsException message: "<<ex.what()<<"\n";
    } catch (const std::runtime_error& ex) {
        std::cout <<ex.what()<<'\n';
    }
    std::cout<<"Hit ENTER to continue\n";
    std::cin.ignore();
    return 0;
}