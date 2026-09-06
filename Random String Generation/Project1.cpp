#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>  // WindowsAPI头文件
#include <conio.h>    // Windows控制台输入输出

// 配置参数
#define STR_LEN 255  //字符串长度
#define TIME_FORMAT "[%04d-%02d-%02d %02d:%02d:%02d]"  //时间格式
#define TIME_STR_LEN 25  //时间专用字符
#define CHAR_SET "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@#^&_/-"  //字符内容（随意添加修改）
#define CHAR_COUNT 69   //字符内容数量
#define FILE_NAME "random_strings.txt"  //输出文件名
#define INIT_CAPACITY 1000   //默认输出间隔（毫秒）
#define INTERVAL_STEP 100    
#define MIN_INTERVAL 100    

// 全局变量
char** all_records = NULL;
int total_count = 0;
int capacity = 0;
FILE* fp = NULL;
int is_paused = 0;
int generate_interval = 1000;  // 生成间隔（毫秒）


// 生成随机字符串
void generate_random_str(char* str) {
    for (int i = 0; i < STR_LEN - 1; i++) {
        str[i] = CHAR_SET[rand() % CHAR_COUNT];
    }
    str[STR_LEN - 1] = '\0';
}


// 检查字符串是否重复
int is_duplicate(char* str) {
    for (int i = 0; i < total_count; i++) {
        if (strcmp(str, all_records[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


// 扩容历史记录数组
void expand_records() {
    if (total_count >= capacity) {
        int new_capacity = (capacity == 0) ? INIT_CAPACITY : capacity * 2;
        char** new_records = (char**)realloc(all_records, new_capacity * sizeof(char*));
        if (new_records == NULL) {
            printf("内存分配失败！\n");
            exit(1);
        }
        all_records = new_records;
        capacity = new_capacity;
    }
}


// 保存到历史记录
void save_to_history(char* str) {
    expand_records();
    all_records[total_count] = (char*)malloc(STR_LEN * sizeof(char));
    if (all_records[total_count] == NULL) {
        printf("内存分配失败！\n");
        exit(1);
    }
    strcpy(all_records[total_count], str);
    total_count++;
}


// 加载历史记录（从文件读取并去重）
void load_existing_records() {
    FILE* read_fp = fopen(FILE_NAME, "r");
    if (read_fp == NULL) {
        // 文件不存在时直接返回
        return;
    }

    char line[STR_LEN + TIME_STR_LEN + 20];  // 容纳一行完整内容
    char temp_str[STR_LEN];
    int dummy;  // 用于接收无关的数字（序号和时间）

    // 解析文件内容，提取随机字符串部分
    while (fgets(line, sizeof(line), read_fp) != NULL) {
        // 匹配格式："第%d条：%s[%d-%d-%d %d:%d:%d]"
        if (sscanf(line, "第%d条：%254s[%d-%d-%d %d:%d:%d]",
            &dummy, temp_str, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy) >= 2) {

            if (!is_duplicate(temp_str)) {
                save_to_history(temp_str);
            }
        }
    }

    fclose(read_fp);
    printf("已加载历史记录 %d 条\n", total_count);
}


// 获取当前时间
void get_current_time(char* time_str) {
    time_t now = time(NULL);
    struct tm local_time;
    localtime_s(&local_time, &now);  // Windows 安全版本的本地时间转换

    sprintf_s(
        time_str,
        TIME_STR_LEN,
        TIME_FORMAT,
        local_time.tm_year + 1900,
        local_time.tm_mon + 1,
        local_time.tm_mday,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec
    );
}


// 释放内存
void free_memory() {
    if (all_records != NULL) {
        for (int i = 0; i < total_count; i++) {
            free(all_records[i]);
        }
        free(all_records);
    }
}


// 关闭文件并释放资源
void close_file() {
    if (fp != NULL) {
        fclose(fp);
        printf("\n文件已保存至：%s\n", FILE_NAME);
    }
    free_memory();
}


// 检查热键
void check_hotkey() {
    if (_kbhit()) {  //检测按键是否按下
        int key = _getch();  //获取按键
        switch (key) {
        case ' ':  // 空格键：暂停/恢复
            is_paused = ~is_paused;
            printf("\n%s生成\n", is_paused ? "已暂停" : "已恢复");
            break;
        case '+':  // 加号：增加间隔
            generate_interval += INTERVAL_STEP;
            printf("\n间隔调整为：%dms\n", generate_interval);
            break;
        case '-':  // 减号：减少间隔
            generate_interval = (generate_interval > MIN_INTERVAL) ? (generate_interval - INTERVAL_STEP) : MIN_INTERVAL;
            printf("\n间隔调整为：%dms\n", generate_interval);
            break;
        case 'q':  // q键：退出程序
            printf("\n正在退出程序...\n");
            exit(0);
            break;
        }
        Sleep(200);  // 防按键连点（200毫秒）
    }
}


int main() {
    char random_str[STR_LEN];
    char time_str[TIME_STR_LEN];
    srand((unsigned int)time(NULL));  // 初始化随机数种子

    // 加载历史记录（程序启动时执行）
    load_existing_records();

    // 以追加模式打开文件（避免覆盖已有内容）
    if (fopen_s(&fp, FILE_NAME, "a") != 0) {
        printf("文件打开失败！\n");
        free_memory();
        return 1;
    }
    atexit(close_file);  // 程序退出时自动关闭文件

    // 显示操作说明
    printf("开始生成随机串（全量去重+附加时间）\n");
    printf("操作说明：\n");
    printf("  空格键：暂停/恢复生成\n");
    printf("  + 键：增加生成间隔（+100ms）\n");
    printf("  - 键：减少生成间隔（-100ms，最低100ms）\n");
    printf("  q 键：退出程序\n");

    // 主循环
    while (1) {
        check_hotkey();  // 检测按键操作

        if (is_paused) {
            Sleep(100);  // 暂停时休眠，降低CPU占用
            continue;
        }

        // 生成不重复的随机字符串
        do {
            generate_random_str(random_str);
        } while (is_duplicate(random_str));

        // 记录并写入文件
        get_current_time(time_str);
        save_to_history(random_str);
        printf("第%d条：%s%s\n", total_count, random_str, time_str);
        fprintf(fp, "第%d条：%s%s\n", total_count, random_str, time_str);
        fflush(fp);  // 立即写入文件，避免数据丢失

        Sleep(generate_interval);  // 等待指定间隔
    }

    return 0;
}