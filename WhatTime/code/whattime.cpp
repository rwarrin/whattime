#include <stdio.h>
#include <stdint.h>

#include <windows.h>

#define NS_PER_SECOND 1000000000LL
#define NS_BUCKETS_PER_SECOND (NS_PER_SECOND / 100)
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct time_config
{
    int32_t Valid;
    int32_t OffsetHours;
    int32_t OffsetMinutes;
    char Name[64];
};

static time_config GlobalTimeConfigs[64] = {0};

inline int32_t
Absolute(int32_t Val)
{
    int32_t Result = ((Val >= 0) ? Val : -Val);
    return(Result);
}

inline int32_t
IsDelimiter(char C)
{
    int32_t Result = (C == '|');
    return(Result);
}

inline int32_t
IsEndOfLine(char C)
{
    int32_t Result = ((C == '\r') ||
                      (C == '\n') ||
                      (C == 0));
    return(Result);
}

inline int32_t
CopyName(char *Dest, int32_t Length, char *Src)
{
    int32_t Result = 0;
    while(!IsDelimiter(*Src) && !IsEndOfLine(*Src))
    {
        *Dest++ = *Src++;
        ++Result;
    }
    return(Result);
}

inline void
ConsumeWhitespace(char **Src)
{
    for(;;)
    {
        char C = **Src;
        if((C == ' ') ||
           (C == '\t') ||
           (C == '\r') ||
           (C == '\n'))
        {
            ++(*Src);
        }
        else
        {
            break;
        }
    }
}

inline void
ConsumeToNextDelimiterOrEOL(char **Src)
{
    while(!IsDelimiter(**Src) && !IsEndOfLine(**Src))
    {
        ++(*Src);
    }
}

inline void
SkipLine(char **Src)
{
    while(!IsEndOfLine(**Src))
    {
        ++(*Src);
    }
}

static void
ReadConfigFromFile(char *FileName, time_config *TimeConfigs, uint32_t Size)
{
    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        int32_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        char *FileContents = (char *)malloc(FileSize + 1);
        fread(FileContents, 1, FileSize, File);
        FileContents[FileSize] = 0;

        char *ReadAt = FileContents;
        int32_t ConfigIndex = 0;
        while(ConfigIndex < Size)
        {
            ConsumeWhitespace(&ReadAt);

            if(*ReadAt == 0)
            {
                break;
            }
            else if(*ReadAt == '#')
            {
                SkipLine(&ReadAt);
            }
            else
            {
                time_config *TimeConfig = TimeConfigs + ConfigIndex++;
                TimeConfig->Valid = true;

                int32_t Length = CopyName(TimeConfig->Name, ArrayCount(TimeConfig->Name), ReadAt);
                ReadAt += Length;
                ConsumeToNextDelimiterOrEOL(&ReadAt);

                if(IsDelimiter(*ReadAt))
                {
                    ++ReadAt;
                    TimeConfig->OffsetHours = atoi(ReadAt);
                    ConsumeToNextDelimiterOrEOL(&ReadAt);
                }

                if(IsDelimiter(*ReadAt))
                {
                    ++ReadAt;
                    TimeConfig->OffsetMinutes = atoi(ReadAt);
                    ConsumeToNextDelimiterOrEOL(&ReadAt);
                }
            }
        }

        free(FileContents);
        fclose(File);
    }
}

static void
PrintHelp(FILE *File, char *Command)
{
    fprintf(File, "Usage: %s [help | list]\n", Command);
    fprintf(File, "Computes locale specific times based on timezone information stored in whattime_cfg.cfg\n");
    fprintf(File, "\thelp\tPrints this help text.\n");
    fprintf(File, "\t    \t\tAlt: -help, --help, /?\n");
    fprintf(File, "\tlist\tLists system known timezone information useful for creating a config file.\n");
}

static void
PrintSystemTimezoneInformation(FILE *File)
{
    DYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZone = {0};
    DWORD TimeZoneResult = 0;
    DWORD TimeZoneIndex = 0;
    fprintf(File, "Standard |    DST   | Name\n");
    fprintf(File, "---------+----------+---------------------------------------\n");
    do
    {
        TimeZoneResult = EnumDynamicTimeZoneInformation(TimeZoneIndex++, &DynamicTimeZone);
        if(TimeZoneResult == ERROR_SUCCESS)
        {
            int32_t AbsStandardBias = Absolute(DynamicTimeZone.Bias);
            int32_t StandardOffsetHours = AbsStandardBias / 60;
            int32_t StandardOffsetMinutes = (AbsStandardBias - (StandardOffsetHours * 60));
            char StandardOffsetSign = ((DynamicTimeZone.Bias < 0) ? '+' : '-');

            int32_t AbsDaylightBias = Absolute(DynamicTimeZone.DaylightBias);
            int32_t DaylightOffsetHours = AbsDaylightBias / 60;
            int32_t DaylightOffsetMinutes = (AbsDaylightBias - (DaylightOffsetHours * 60));
            char DaylightOffsetSign = ((DynamicTimeZone.DaylightBias < 0) ? '+' : '-');

            fprintf(File, "%c%02dh %02dm | %c%02dh %02dm | %ws\n",
                    StandardOffsetSign, StandardOffsetHours, StandardOffsetMinutes,
                    DaylightOffsetSign, DaylightOffsetHours, DaylightOffsetMinutes,
                    DynamicTimeZone.StandardName);
        }
    } while(TimeZoneResult != ERROR_NO_MORE_ITEMS);

}

int main(int ArgCount, char **Args)
{
    if(ArgCount != 1)
    {
        for(int32_t ArgIndex = 1; ArgIndex < ArgCount; ++ArgIndex)
        {
            if((!strcmp(Args[ArgIndex], "help")) ||
               (!strcmp(Args[ArgIndex], "-help")) ||
               (!strcmp(Args[ArgIndex], "--help")) ||
               (!strcmp(Args[ArgIndex], "/?")))
            {
                PrintHelp(stdout, Args[0]);
            }
            else if(!strcmp(Args[ArgIndex], "list"))
            {
                PrintSystemTimezoneInformation(stdout);
            }
        }
    }
    else
    {
        ReadConfigFromFile("whattime_config.dat", GlobalTimeConfigs, ArrayCount(GlobalTimeConfigs));

        FILETIME OriginalSystemFileTime = {0};
        GetSystemTimeAsFileTime(&OriginalSystemFileTime);
        ULARGE_INTEGER OriginalSystemLargeInt = {OriginalSystemFileTime.dwLowDateTime, OriginalSystemFileTime.dwHighDateTime};

        for(uint32_t ConfigIndex = 0; ConfigIndex < ArrayCount(GlobalTimeConfigs); ++ConfigIndex)
        {
            time_config *Config = GlobalTimeConfigs + ConfigIndex;
            if(Config->Valid)
            {
                FILETIME SystemFileTime = OriginalSystemFileTime;
                ULARGE_INTEGER SystemLargeInt = OriginalSystemLargeInt;

                int64_t Offset = ((Config->OffsetHours * MINUTES_PER_HOUR * SECONDS_PER_MINUTE * NS_BUCKETS_PER_SECOND) +
                                  (Config->OffsetMinutes * SECONDS_PER_MINUTE * NS_BUCKETS_PER_SECOND));
                SystemLargeInt.QuadPart = SystemLargeInt.QuadPart + Offset;
                SystemFileTime.dwHighDateTime = SystemLargeInt.HighPart;
                SystemFileTime.dwLowDateTime = SystemLargeInt.LowPart;

                SYSTEMTIME ComputedSystemTime = {0};
                FileTimeToSystemTime(&SystemFileTime, &ComputedSystemTime);
                printf("%02d/%02d/%02d %02d:%02d:%02d %s\n",
                       ComputedSystemTime.wMonth,
                       ComputedSystemTime.wDay,
                       ComputedSystemTime.wYear,
                       ComputedSystemTime.wHour,
                       ComputedSystemTime.wMinute,
                       ComputedSystemTime.wSecond,
                       Config->Name);
            }
        }
    }

    return(0);
}
