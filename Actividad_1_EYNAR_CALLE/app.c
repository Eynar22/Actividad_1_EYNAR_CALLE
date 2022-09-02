#include <stdio.h>
#include <pthread.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"



/* Local includes. */
#include "console.h"
#include <termios.h>




#define TASK1_PRIORITY 1 // led verde
#define TASK2_PRIORITY 2 //led rojo
#define TASK3_PRIORITY 2 // lectura

#define BLACK "\033[30m" /* Black */
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */
#define DISABLE_CURSOR() printf("\e[?25l")
#define ENABLE_CURSOR() printf("\e[?25h")

#define clear() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

typedef struct
{
    int pos;
    char *color;
    int period_ms;
} st_led_param_t;

st_led_param_t green = {
    6,
    GREEN,
    250};  //250ms
st_led_param_t red = {
    13,
    RED,
    100};  //100ms

TaskHandle_t greenTask_hdlr, redTask_hdlr,getCharTask_hdlr;



static void prvTask_getChar(void *pvParameters)
{
    char key;
    int n;
    uint32_t notificationValue_getChar;

    /* I need to change  the keyboard behavior to
    enable nonblock getchar */
    struct termios initial_settings,
        new_settings;

    tcgetattr(0, &initial_settings);

    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1;

    tcsetattr(0, TCSANOW, &new_settings);
    /* End of keyboard configuration */
    for (;;)
    {
        int stop = 0;
        key = getchar();

        if (key=='1'||key=='2'||key=='3'||key=='4'||key=='5'||key=='6'||key=='7'||key=='8'||key=='9'||key=='0')
        {
            key='n'; 

        }
        xTaskNotifyWait(
            0x00, 
            0x00, 
            &notificationValue_getChar,
            1);
            
        if(notificationValue_getChar == 99)
        {   switch (key)
                {
                    case '+': //vuelve al funcionamento normal 
                        vTaskResume(greenTask_hdlr); 
                        vTaskResume(redTask_hdlr);   
                        vTaskResume(getCharTask_hdlr);  
                        xTaskNotify(greenTask_hdlr, 0UL, eSetValueWithOverwrite);    
                        xTaskNotify(redTask_hdlr, 0UL, eSetValueWithOverwrite);      
                        xTaskNotify(getCharTask_hdlr, 0UL, eSetValueWithOverwrite);  
                        break;
                }
        }
        else
        {
            switch (key)
            {
                case 'n': // led rojo por 100ms
                    vTaskResume(redTask_hdlr);
                    break;
                case '*': //led verde permance apagado
                {
                    xTaskNotify(getCharTask_hdlr, 99UL, eSetValueWithOverwrite); 
                    xTaskNotify(greenTask_hdlr, 1UL, eSetValueWithOverwrite);    
                    vTaskResume(redTask_hdlr);                                   
                    xTaskNotify(redTask_hdlr, 1UL, eSetValueWithOverwrite);          
                    break;
                }
                case 'k': //interrumpe el programa
                    stop = 1;
                    break;
            }
        }
        if (stop)
        {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    tcsetattr(0, TCSANOW, &initial_settings);
    ENABLE_CURSOR();
    exit(0);
    vTaskDelete(NULL);
}

static void prvTask_red_led(void *pvParameters)
{
    uint32_t notificationValue_red;
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
         //enciende Led
        gotoxy(led->pos, 2);
        printf("%s⬤", led->color);
        fflush(stdout);
        vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
        if (xTaskNotifyWait(
            ULONG_MAX,
            ULONG_MAX,
            &notificationValue_red,                   
            1)) 
            {
                if(notificationValue_red == 01)       
                {
                   vTaskSuspend(redTask_hdlr);        
                }
            }

        //apaga led
        gotoxy(led->pos, 2);
        printf("%s⬤", BLACK);
        fflush(stdout);

        vTaskSuspend(redTask_hdlr);

    }

    vTaskDelete(NULL);
}



static void prvTask_green_led(void *pvParameters)
{
    uint32_t notificationValue_green;
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        //enciende el Led  
        gotoxy(led->pos, 2);
        printf("%s⬤", led->color);
        fflush(stdout);
        vTaskDelay(led->period_ms / portTICK_PERIOD_MS);

        //Apaga el Led
        gotoxy(led->pos, 2);
        printf("%s⬤", BLACK);
        fflush(stdout);
        vTaskDelay(led->period_ms / portTICK_PERIOD_MS); 
        if (xTaskNotifyWait(
            ULONG_MAX,
            ULONG_MAX,
            &notificationValue_green,
            1)) 
            {
                if(notificationValue_green == 01)       
                {
                   vTaskSuspend(greenTask_hdlr);        
                }
            }

    }

    vTaskDelete(NULL);
}






void app_run(void)
{

    clear();
    DISABLE_CURSOR();
    printf(
        "╔═════════════════╗\n"
        "║                 ║\n"
        "╚═════════════════╝\n");

   
     
    xTaskCreate(prvTask_green_led, "LED_green", configMINIMAL_STACK_SIZE, &green, TASK1_PRIORITY, &greenTask_hdlr);
    xTaskCreate(prvTask_red_led, "LED_red", configMINIMAL_STACK_SIZE, &red, TASK2_PRIORITY, &redTask_hdlr);   
    xTaskCreate(prvTask_getChar, "Get_key", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, &getCharTask_hdlr);
    vTaskSuspend(redTask_hdlr); 

 
    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks      to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for (;;)
    {
    }
}