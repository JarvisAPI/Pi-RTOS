typedef struct TaskInfo_s
{
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char *name;
} TaskInfo_t;

TaskInfo_t tasks[] =
{
{818, 71000, 71000, "Task 1"},
{8, 2000, 2000, "Task 2"},
{44, 44000, 44000, "Task 3"},
{112, 22000, 22000, "Task 4"},
{12, 5000, 5000, "Task 5"},
{7, 1000, 1000, "Task 6"},
{264, 52000, 52000, "Task 7"},
{12, 1000, 1000, "Task 8"},
{23, 2000, 2000, "Task 9"},
{211, 26000, 26000, "Task 10"},
{97, 11000, 11000, "Task 11"},
{280, 43000, 43000, "Task 12"},
{272, 30000, 30000, "Task 13"},
{9, 1000, 1000, "Task 14"},
{7, 1000, 1000, "Task 15"},
{290, 41000, 41000, "Task 16"},
{933, 53000, 53000, "Task 17"},
{579, 56000, 56000, "Task 18"},
{49, 10000, 10000, "Task 19"},
{29, 35000, 35000, "Task 20"},
{171, 17000, 17000, "Task 21"},
{71, 19000, 19000, "Task 22"},
{9, 8000, 8000, "Task 23"},
{6, 2000, 2000, "Task 24"},
{63, 14000, 14000, "Task 25"},
{35, 9000, 9000, "Task 26"},
{1290, 54000, 54000, "Task 27"},
{14, 19000, 19000, "Task 28"},
{37, 1000, 1000, "Task 29"},
{110, 27000, 27000, "Task 30"},
{73, 10000, 10000, "Task 31"},
{219, 23000, 23000, "Task 32"},
{159, 72000, 72000, "Task 33"},
{11, 1000, 1000, "Task 34"},
{16, 6000, 6000, "Task 35"},
{186, 23000, 23000, "Task 36"},
{6, 2000, 2000, "Task 37"},
{59, 3000, 3000, "Task 38"},
{375, 10000, 10000, "Task 39"},
{6, 1000, 1000, "Task 40"},
{6, 1000, 1000, "Task 41"},
{62, 9000, 9000, "Task 42"},
{15, 9000, 9000, "Task 43"},
{202, 7000, 7000, "Task 44"},
{32, 30000, 30000, "Task 45"},
{8, 1000, 1000, "Task 46"},
{116, 12000, 12000, "Task 47"},
{25, 13000, 13000, "Task 48"},
{85, 35000, 35000, "Task 49"},
{538, 56000, 56000, "Task 50"},
{154, 27000, 27000, "Task 51"},
{10, 8000, 8000, "Task 52"},
{52, 31000, 31000, "Task 53"},
{23, 10000, 10000, "Task 54"},
{38, 3000, 3000, "Task 55"},
{6, 11000, 11000, "Task 56"},
{247, 30000, 30000, "Task 57"},
{75, 6000, 6000, "Task 58"},
{198, 31000, 31000, "Task 59"},
{152, 19000, 19000, "Task 60"},
{26, 6000, 6000, "Task 61"},
{90, 18000, 18000, "Task 62"},
{2446, 94000, 94000, "Task 63"},
{25, 6000, 6000, "Task 64"},
{30, 15000, 15000, "Task 65"},
{12, 1000, 1000, "Task 66"},
{78, 3000, 3000, "Task 67"},
{19, 1000, 1000, "Task 68"},
{19, 4000, 4000, "Task 69"},
{13, 1000, 1000, "Task 70"},
{288, 23000, 23000, "Task 71"},
{42, 16000, 16000, "Task 72"},
{11, 10000, 10000, "Task 73"},
{29, 74000, 74000, "Task 74"},
{21, 6000, 6000, "Task 75"},
{13, 12000, 12000, "Task 76"},
{886, 35000, 35000, "Task 77"},
{515, 30000, 30000, "Task 78"},
{176, 43000, 43000, "Task 79"},
{23, 6000, 6000, "Task 80"},
{198, 31000, 31000, "Task 81"},
{426, 20000, 20000, "Task 82"},
{126, 4000, 4000, "Task 83"},
{171, 40000, 40000, "Task 84"},
{6, 1000, 1000, "Task 85"},
{183, 17000, 17000, "Task 86"},
{6, 1000, 1000, "Task 87"},
{52, 23000, 23000, "Task 88"},
{12, 1000, 1000, "Task 89"},
{806, 53000, 53000, "Task 90"},
{80, 8000, 8000, "Task 91"},
{65, 68000, 68000, "Task 92"},
{24, 2000, 2000, "Task 93"},
{73, 49000, 49000, "Task 94"},
{1217, 65000, 65000, "Task 95"},
{376, 21000, 21000, "Task 96"},
{80, 50000, 50000, "Task 97"},
{10, 1000, 1000, "Task 98"},
{60, 5000, 5000, "Task 99"},
{21, 7000, 7000, "Task 100"}
};