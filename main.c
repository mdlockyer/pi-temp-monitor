#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <wchar.h>
#include <locale.h>
#include <argp.h>

// MARK: Arg parsing

static struct argp_option options[] = {
    {"fahrenheit", 'f', 0, 0, "Display temperature in Fahrenheit rather than the default Celsius"},
    {"length", 'l', "LENGTH", 0, "The length of the display bar"},
    {"interval", 'i', "INTERVAL", 0, "The frequency(in seconds) that the display is refreshed"},
    {"file-path", 'p', "FILE", 0, "The log file from which temperature data is read"},
    {0}};

struct arguments
{
    char fahrenheit;
    int length;
    float interval;
    char *filePath;
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{

    struct arguments *arguments = state->input;
    switch (key)
        {
        case 'f':
            arguments->fahrenheit = 1;
            break;
        case 'l':
            arguments->length = atoi(arg);
            break;
        case 'i':
            arguments->interval = (float)atof(arg);
            break;
        case 'p':
            arguments->filePath = arg;
            break;
        case ARGP_KEY_ARG:
            break;
        case ARGP_KEY_END:
            break;
        }
    return 0;
}

static char doc[] = "A lighweight program for monitoring the CPU "
                    "temperature of the Raspberry Pi.";
static char args_doc[] = "";

static struct argp argp = { options, parse_opt, args_doc, doc };

// MARK: Helper functions

unsigned long int getReading(char filePath[2048])
{
    FILE *file;
    file = fopen(filePath, "r");
    if (file == NULL)
    {
        printf("\033[0;31mThis system is not supported by PiTempMonitor\033[0m\n");
        exit(EXIT_FAILURE);
    }

    char line[128];
    fgets(line, sizeof(line), file);
    fclose(file);

    unsigned long int lineVal = atoi(line);
    return lineVal;
}

void createDisplayBar(char dest[], float current, float total, int length, float cyanThreshold,
                      float yellowThreshold, float minTemp, float maxTemp)
{
    if (cyanThreshold >= yellowThreshold)
    {
        printf("Expected cyan threshold to be less than yellow threshold");
        exit(EXIT_FAILURE);
    }

    int filledLength = round((float)length * current / total);
    int cyanLength = (float)length * cyanThreshold;
    int yellowLength = (float)length * yellowThreshold;

    int i;
    for (i = 0; i <= length - 1; i++)
    {
        if (i < filledLength)
        {
            if (i < cyanLength && cyanThreshold > 0)
            {
                char blockSymbol[32];
                sprintf(blockSymbol, "\033[0;36m%lc\033[0m", (wint_t)9608);
                i == 0 ? strcpy(dest, blockSymbol) : strcat(dest, blockSymbol);
            }
            else if (i < yellowLength  && yellowThreshold > 0)
            {
                char blockSymbol[32];
                sprintf(blockSymbol, "\033[0;33m%lc\033[0m", (wint_t)9608);
                i == 0 ? strcpy(dest, blockSymbol) : strcat(dest, blockSymbol);
            }
            else
            {
                char blockSymbol[32];
                sprintf(blockSymbol, "\033[0;31m%lc\033[0m", (wint_t)9608);
                i == 0 ? strcpy(dest, blockSymbol) : strcat(dest, blockSymbol);
            }
        }
        else
        {
            char blockSymbol[32];
            sprintf(blockSymbol, "\033[0;30m%lc\033[0m", (wint_t)9608);
            strcat(dest, blockSymbol);
        }
    }
}

int main(int argc, char *argv[])
{

    setlocale(LC_CTYPE, "");

    // Initialize our arguments struct and set default values
    struct arguments arguments;
    arguments.fahrenheit = 0;
    arguments.length = 60;
    arguments.interval = 1.0;
    arguments.filePath = "/sys/class/thermal/thermal_zone0/temp";

    // Parse arguments
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    float cyanThreshold = 0.5;
    float yellowThreshold = 0.725;
    // Set up our temp formatting values for both C and F
    float maxTemp;
    float minTemp;
    char tempUnitsChar;
    if (arguments.fahrenheit) 
    {
        maxTemp = 185.0;
        minTemp = 32.0;
        tempUnitsChar = 'F';
    }
    else
    {
        maxTemp = 85;
        minTemp = 0;
        tempUnitsChar = 'C';
    }

    while (1)
    {
        unsigned long int temp = getReading(arguments.filePath);
        // Format the raw temperature data into C units and cast to float.
        float formattedTemp = (float)temp / 1000.0;
        // Convert to F if required
        if (arguments.fahrenheit) 
        {
            formattedTemp = formattedTemp * 1.8 + 32;
        }

        // Construct the display bar string
        float barCurrent = formattedTemp - minTemp;
        float barTotal = maxTemp - minTemp;
        // Generate our display bar
        char tempBar[2048];
        createDisplayBar(tempBar,
                         barCurrent,
                         barTotal,
                         arguments.length,
                         cyanThreshold,
                         yellowThreshold,
                         minTemp,
                         maxTemp);

        // Combine sub-strings and print
        printf("|%s| CPU temperature: %.1f%lc %C \r",
               tempBar,
               formattedTemp,
               (wint_t)176,
               tempUnitsChar);

        // Flush stdout to file. Without this,
        // we don't get reliably timed prints
        fflush(stdout);
        usleep(arguments.interval * 1e6);
    }
    return 0;
}
