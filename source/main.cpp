#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <iostream>
#include <windows.h>
#include <string>
#include <lmcons.h>
#include <cwchar>
#include <limits>

// ASCII characters from darkest to brightest
const static std::string chars = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
const int CharsAmount = int(chars.length()) - 1;

#define LIGHT_GRAY 7
#define GRAY 8
#define WHITE 15

// an array that stores the precomputed brightness value for each possible RGB value
// i know its not memory efficient because its about 67 megabytes, but at least 
// the program doesn't have to do expensive floating point calculations during video playback
float brightnessLookup[256 * 256 * 256];

// Rec. 601 standard
constexpr float WeightR = 0.299f;
constexpr float WeightG = 0.587f;
constexpr float WeightB = 0.114f;

#define R 2
#define G 1
#define B 0

// The darkest is 0 and the brightest is 1
#define FindBrightness(r, g, b) ((r / 255.0f * WeightR) + (g / 255.0f * WeightG) + (b / 255.0f * WeightB))

// Console window size in characters
// Each character has 16 pixels in height and 8 pixels in width by default (in "Consolas" font)
void GetWindowSize(HANDLE OutputBufferHandle, int& width_out, int& height_out)
{
    CONSOLE_SCREEN_BUFFER_INFO OutputBufferInfo = {};
    GetConsoleScreenBufferInfo(OutputBufferHandle, &OutputBufferInfo);
    width_out = OutputBufferInfo.srWindow.Right - OutputBufferInfo.srWindow.Left + 1;
    height_out = OutputBufferInfo.srWindow.Bottom - OutputBufferInfo.srWindow.Top + 1;
}

void LoadImageToBuffer(cv::Mat& SourceImage, CHAR_INFO* DestBuffer, int WinWidth, int WinHeight)
{
    if (WinWidth < 1 || WinHeight < 1) return;

    // shrink the frame to the size of the console window
    cv::resize(SourceImage, SourceImage, cv::Size(WinWidth, WinHeight), 0.0, 0.0, cv::INTER_NEAREST);
    
    for (int y = 0; y < WinHeight; ++y)
    {
        for (int x = 0; x < WinWidth; ++x)
        {
            const unsigned char* CurrentPix = SourceImage.ptr<unsigned char>(y) + 3 * x;
            CHAR_INFO& CurrentChar = DestBuffer[WinWidth * y + x];
            unsigned int combined_RGB = 
                (unsigned int(CurrentPix[R]) << 16) |
                (unsigned int(CurrentPix[G]) << 8) |
                unsigned int(CurrentPix[B]);
            float CurrentBrightness = brightnessLookup[combined_RGB];
            CurrentChar.Char.AsciiChar = chars[int(CharsAmount * CurrentBrightness)];

            if (CurrentBrightness <= 0.33f) CurrentChar.Attributes = GRAY;
            else if (CurrentBrightness <= 0.66f) CurrentChar.Attributes = LIGHT_GRAY;
            else CurrentChar.Attributes = WHITE;
        }
    }
}

// get username in order to access their desktop
std::string GetUsername()
{
    TCHAR username[UNLEN + 1] = {};
	DWORD size = UNLEN + 1;
	std::string current_username = "";
	GetUserName((TCHAR*)username, &size);
	for (unsigned long i = 0; i < size; ++i) {
		current_username += (char)username[i];
	}
	return current_username;
}

// precomputes the brightness value for each possible RGB value
void ComputeBrightnessLookupTable()
{
    for (int r = 0; r < 256; ++r) {
        for (int g = 0; g < 256; ++g) {
            for (int b = 0; b < 256; ++b)
            {
                int index = (r << 16) | (g << 8) | b;
                brightnessLookup[index] = FindBrightness(r, g, b);
            }
        }
    }
}

// returns true if str contains substr
bool StrContains(const std::string& str, const std::string& substr)
{
    return str.find(substr) >= 0 && str.find(substr) < str.length();
}

void ProcessUserInput(std::string& Input)
{
    // remove all spaces at the beginning of a string
    for (int i = 0; Input[i] != '\\' && i < Input.length(); ++i)
    {
        if(Input[i] == ' ') Input.erase(0, 1);
    }

    // erase all zeros that somehow appear in the string but i don't know why
    for (int i = 0; i < Input.length(); ++i) {
        if (int(Input[i]) == 0) {
            Input.erase(i, 1);
            --i;
        }
    }
}

void RenderBuffer(CHAR_INFO* buffer, HANDLE ConsoleHandle, short win_width, short win_height)
{
    SMALL_RECT writeArea = { 0, 0, win_width - 1, win_height - 1 };
    WriteConsoleOutputA(ConsoleHandle, buffer, { win_width, win_height }, { 0, 0 }, &writeArea);
}

void ChangeConsoleTextSize(HANDLE ConsoleHandle, int new_width, int new_height, const wchar_t* font_name = L"Consolas")
{
    CONSOLE_FONT_INFOEX cfi = {};
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = new_width;
    cfi.dwFontSize.Y = new_height;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, font_name);
    SetCurrentConsoleFontEx(ConsoleHandle, FALSE, &cfi);
}

void askResolution(HANDLE ConsoleHandle)
{
    char resolution = 0;

    std::cout << "\n\nSelect console window resolution\n"
        << "You can also select any font you want in your console window settings\n\n"
        << "0 - Keep as is\n"
        << "1 - Best resolution (2x5 pixels in a charachter)\n"
        << "2 - High resolution (4x8 pixels in a charachter)\n"
        << "3 - Medium resolution (6x12 pixels in charachter)\n"
        << "4 - Low resolution (8x16 pixels in charachter)\n\n"
        << "Enter the number of desired resolution : ";

    std::cin >> resolution;

    // i replaced max function with a pointer because it conflicts with
    // #define max(a,b) (((a) > (b)) ? (a) : (b))
    // in minwindef.h and nothing else seems to work
    long long (*i_hate_cpp)() = std::numeric_limits<std::streamsize>::max;
    const long long max_streamsize = (*i_hate_cpp)();
    std::cin.ignore(max_streamsize, '\n');

    for (bool passed = false; !passed; passed = true)
    {
        switch (resolution) {
        case '0':
            break;
        case '1':
            ChangeConsoleTextSize(ConsoleHandle, 2, 5);
            break;
        case '2':
            ChangeConsoleTextSize(ConsoleHandle, 4, 8);
            break;
        case '3':
            ChangeConsoleTextSize(ConsoleHandle, 6, 12);
            break;
        case '4':
            ChangeConsoleTextSize(ConsoleHandle, 8, 16);
            break;
        default:
            std::cout << "Bad input, try again : ";
            std::cin >> resolution;
            std::cin.ignore(max_streamsize, '\n');
            passed = false;
        }
    }
}

std::string askVideoPath()
{
    std::string username = GetUsername();
    // this desktop path is usually works
    std::string DesktopPath = "C:\\Users\\" + username + "\\Desktop\\";
    std::string video_path;

    std::cout
        << "Insert the path of your video\n\n"
        << "Example :\nC:\\folder_name\\some_other_folder\\videofile.mp4\n\n"
        << "If the video is on your desktop, just type \"-d\"\n"
        << "(You can resize the console window as you wish, even while the video is playing)\n\n";

    do {
        std::getline(std::cin, video_path, '\n');
    } while (video_path.empty());

    // read from desktop
    if (StrContains(video_path, "-d")) {
        video_path.clear();
        std::cout
            << "\nNow insert the name of your video with its extension\n"
            << "Example : videofile.mp4 (any other extension is also valid)\n";
        do {
            std::getline(std::cin, video_path, '\n');
        } while (video_path.empty());
        video_path = DesktopPath + video_path;
    }
    return video_path;
}

int main(void)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ComputeBrightnessLookupTable();
    ChangeConsoleTextSize(hConsole, 8, 16);
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

    std::string video_path = askVideoPath();
    ProcessUserInput(video_path);

    std::cout << "Opening video at " << video_path << '\n';

    // Console window size in characters
    int win_width;
    int win_height;
    GetWindowSize(hConsole, win_width, win_height);

    // clear screen
    DWORD charsWritten;
    FillConsoleOutputCharacterA(hConsole, (TCHAR)' ', win_width * win_height, { 0, 0 }, &charsWritten);
    SetConsoleCursorPosition(hConsole, { 0, 0 });

    cv::VideoCapture SourceVideo(video_path);

    if (!SourceVideo.isOpened())
    {
        std::cout << "The video at " << video_path << " was not found.\n";
        std::cout << "This address is probably invalid or the file extension is not supported\n";
        std::cout << "\nPress Enter to exit . . . ";
        std::cin.get();
        return -1;
    }

    long long frame_amount = static_cast<long long>(SourceVideo.get(cv::CAP_PROP_FRAME_COUNT));
    double original_fps = SourceVideo.get(cv::CAP_PROP_FPS);
    if (original_fps <= 0.0000001 || frame_amount <= 0) {
        std::cout 
            << "There was an error with this particular video, please try another one"
            << "\nPress Enter to exit . . . ";
        std::cin.get();
    }

    std::cout
        << "Video path - " << video_path
        << "\n\nOriginal frame rate of the video - " << original_fps
        << "\nOriginal resolution of the video - " << SourceVideo.get(cv::CAP_PROP_FRAME_WIDTH) << " x " << SourceVideo.get(cv::CAP_PROP_FRAME_HEIGHT);

    int overall_sec = int(frame_amount / original_fps);
    int overall_min = overall_sec / 60;
    int sec = overall_sec % 60;
    int min = overall_min % 60;
    int hrs = overall_min / 60;

    std::cout << "\n\nVideo length - "
        << (hrs < 10 ? '0' : '\0') << hrs << " : "
        << (min < 10 ? '0' : '\0') << min << " : "
        << (sec < 10 ? '0' : '\0') << sec;

    askResolution(hConsole);

    LARGE_INTEGER frame_end_time;
    LARGE_INTEGER frame_begin_time;
    double performance_frequency;
    {
        LARGE_INTEGER perf;
        QueryPerformanceFrequency(&perf);
        performance_frequency = double(perf.QuadPart);
    }
    double DeltaTime;
    const double MAX_DELTA_TIME = 1.0;
    const double expected_DeltaTime = 1.0 / original_fps;
    double DeltaTime_counter = expected_DeltaTime;

    GetWindowSize(hConsole, win_width, win_height);
    CHAR_INFO* imageBuffer = new CHAR_INFO[win_width * win_height];

    int last_width = win_width;
    int last_height = win_height;

    // current captured frame
    cv::Mat current;

    SourceVideo.read(current);
    LoadImageToBuffer(current, imageBuffer, win_width, win_height);

    for (long long current_frame_number = 1; current_frame_number < frame_amount;)
    {
        QueryPerformanceCounter(&frame_begin_time);

        GetWindowSize(hConsole, win_width, win_height);

        // if the width or height of the last frame does not match the current frame
        // it means the window has been resized
        while (last_width != win_width || last_height != win_height)
        {
            last_width = win_width;
            last_height = win_height;
            delete[] imageBuffer;
            imageBuffer = new CHAR_INFO[win_width * win_height];
            GetWindowSize(hConsole, win_width, win_height);
        }
        
        QueryPerformanceCounter(&frame_end_time);
        DeltaTime = (frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
        if (DeltaTime > MAX_DELTA_TIME) DeltaTime = MAX_DELTA_TIME;

        DeltaTime_counter -= DeltaTime;

        // prevents the program from running faster than the original video
        if (DeltaTime_counter < 0.0)
        {
            // prevents the program from running slower than the original video
            int leftover_frames = int(fabs(DeltaTime_counter) / expected_DeltaTime);
            for (int skip = leftover_frames == 0 ? 1 : leftover_frames; skip > 0; --skip) {
                SourceVideo.read(current);
                ++current_frame_number;
            }
            if (current.empty()) continue;
            LoadImageToBuffer(current, imageBuffer, win_width, win_height);
            SetConsoleCursorPosition(hConsole, { 0, 0 });
            RenderBuffer(imageBuffer, hConsole, win_width, win_height);
            DeltaTime_counter = expected_DeltaTime;

            QueryPerformanceCounter(&frame_end_time);
            DeltaTime = (frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
            if (DeltaTime > MAX_DELTA_TIME) DeltaTime = MAX_DELTA_TIME;
            DeltaTime_counter -= DeltaTime;
        }

        last_width = win_width;
        last_height = win_height;
    }
    
    // clear screen
    FillConsoleOutputCharacterA(hConsole, (TCHAR)' ', win_width * win_height, { 0, 0 }, &charsWritten);

    ChangeConsoleTextSize(hConsole, 8, 16);
    std::cout << "The video was played successfully\n\nPress Enter to exit . . . ";;

    std::cin.get();

    return 0;
}
