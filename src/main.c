// 6 points distributed: To Joose 4, Jasu 1 and Aulis 1
// Joose Kalliokoski mainly wrote and cleaned the code
// Jasu Niskanen wrote some code and the plan for the project
// Aulis Neitiniemi contributed to the code and shot and edited the video
// We all participated on planning the functionality
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 
#define BUFFER_SIZE 100
//Aulis was here
//Add here necessary states
enum state { IDLE=1, WRITE/*, DISPLAY*/}; // Initializing FSM and the state variable
enum state programState = IDLE;
char dot = '.';
char dash = '-';
char space = ' ';
/*The message-array was originally intended for displayTask functionality, but we didn't dare touch it this close to 
the deadline. The program should function without it.*/
char message[BUFFER_SIZE];


/*
We realized we did not need this task for Tier 1 implementation
static void displayTask(void *pvParameters) {
    int spaceCount = 0;
    while(1){
        if(programState == DISPLAY) {
            printf("Displaying message: \n");
            
            for(size_t i = 0; message[i] != '\0'; i++) {
                if(message[i] == space) {
                    spaceCount++;
                    if(spaceCount==2) {
                        spaceCount = 0;
                        programState =IDLE;
                        message[i] = '\0';
                        printf("%s\n", message);
                        break;
                    }
                } else {
                    spaceCount = 0;
                }
            }
        } 
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
*/
void writeTask(void *pvParameters) {
    (void)pvParameters;

    
    float ax, ay, az, gx, gy, gz, t;
    // Setting up the sensor. 
    if (init_ICM42670() == 0) {
        //usb_serial_print("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
           // usb_serial_print("ICM-42670P could not initialize accelerometer or gyroscope");
        }
        /*int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
        usb_serial_print ("Enable gyro: %d\n",_enablegyro);
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        usb_serial_print ("Gyro return:  %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        usb_serial_print ("Accel return:  %d\n", _accel);*/
    } else { // This is not supposed to be reached
       // usb_serial_print("Failed to initialize ICM-42670P.\n");
    }
    // Start collection data here. Infinite loop. 
    int spaceCount = 0;

    while (1)
    {
        if(programState == WRITE) {
            for(int i=0;i<BUFFER_SIZE;i++) {
                vTaskDelay(pdMS_TO_TICKS(500));
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                    if(ay > 0.60) { // Tarkastetaan onko y-akselin arvo yli 0.60 (viiva)
                        printf("-\n");
                        message[i] = dash;
                        spaceCount = 0;
                    } else if(ax < -0.6) { // Tarkastetaan onko x-akselin arvo alle -0.60 (piste)
                        message[i] = dot;
                        printf("*\n");
                        spaceCount = 0;
                    } else if(ax > 0.6){ // Tarkastetaan onko x-akselin arvo yli 0.60 (välilyönti)
                        printf("' '\n");
                        message[i] = space;
                        spaceCount++;
                        if(spaceCount==2) { // Kahden peräkkäisen välilyönnin jälkeen vaihdetaan tilaa IDLE
                            printf("Sen pituinen se!\n");
                            programState = IDLE;
                            spaceCount = 0;
                            break;
                            }
                        }
                    }
            }  
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void buttonFxn(uint gpio, uint32_t eventMask) { // IDLE -> WRITE
    programState = WRITE;
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    }
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    gpio_init(BUTTON1); // Initializing and setting up a button interrupt
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, &buttonFxn);

    //TaskHandle_t myDisplayTask = NULL;
    TaskHandle_t myWriteTask = NULL;

    
    /*BaseType_t displayResult = xTaskCreate(displayTask,       // (en) Task function
                "display",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &myDisplayTask);    // (en) A handle to control the execution of this task
                */
    BaseType_t writeResult = xTaskCreate(writeTask,       // (en) Task function
                "write",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &myWriteTask);    // (en) A handle to control the execution of this task



    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}


