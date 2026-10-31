#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "script.h"

// 控制台字体颜色
#define COLOR_RED     "\033[31m"    // 操作失败
#define COLOR_GREEN   "\033[32m"    // 操作成功
#define COLOR_YELLOW "\033[1;33m"   // 执行操作
#define COLOR_RESET   "\033[0m"     // 重置颜色

// 把一行字做成贴图；同时把贴图尺寸写回 outDst
static SDL_Texture* createLineTexture(SDL_Renderer* ren, TTF_Font* font,
    const char* text, SDL_FRect* outDst)
{
    SDL_Surface* surf = TTF_RenderText_Blended(font, text, 0, { 235, 235, 235, 255 });
    if (!surf) {
        printf(COLOR_RED "RenderText failed: " COLOR_RESET "%s\n", SDL_GetError());
        return nullptr; 
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    outDst->w = (float)surf->w;
    outDst->h = (float)surf->h;
    SDL_DestroySurface(surf);          // surf 用完就扔，留着占内存
    return tex;
}

int main(int, char**){
    // 文本编码类型设为UTF-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // 读取剧本文件
    const char* base = SDL_GetBasePath();
    std::string scriptPath = std::string(base) + "assets\\script.txt";
    printf(COLOR_YELLOW "Access the script file path:" COLOR_RESET "%s\n", scriptPath.c_str());

    //加载剧本，统计行数
    std::vector<dialogue> script = loadScript(scriptPath);
    int msgCount = script.size();

    //运行前的排查
    if (msgCount == 0) {
        printf(COLOR_RED "loadScript failed: " COLOR_RESET "%s\n", scriptPath.c_str());
        return -1;
    }
    else {
        printf(COLOR_GREEN "loadScript succeeded.\n" COLOR_RESET);
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf(COLOR_RED "SDL_Init failed: " COLOR_RESET "%s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* win = SDL_CreateWindow("奶龙的Galgame人生", 1280, 720, 0);
    if (!win) {
        printf(COLOR_RED "CreateWindow failed: " COLOR_RESET "%s\n", SDL_GetError());
        return -1; 
    }
    SDL_Renderer* ren = SDL_CreateRenderer(win, nullptr);
    if (!ren) {
        printf(COLOR_RED "CreateRenderer failed: " COLOR_RESET "%s\n", SDL_GetError());
        return -1; 
    }

    if (!TTF_Init()) {
        printf(COLOR_RED "TTF_Init failed: " COLOR_RESET "%s\n", SDL_GetError());
        return -1;
    }
    TTF_Font* font = TTF_OpenFont("C:/Windows/Fonts/msyh.ttc", 40.0f);
    if (!font) {
        printf(COLOR_RED "OpenFont failed: " COLOR_RESET "%s\n", SDL_GetError());
        return -1; 
    }

    // 状态机
    int cursor = 0;

    SDL_FRect speakerDst{ 80.0f, 460.0f, 0.0f, 0.0f };
    SDL_FRect textDst{ 80.0f, 560.0f, 0.0f, 0.0f };
    SDL_Texture* speakerTex = createLineTexture(ren, font, script[cursor].speaker.c_str(), &speakerDst);
    SDL_Texture* textTex = createLineTexture(ren, font, script[cursor].text.c_str(), &textDst);
    printf("[%d/%d] %s: %s\n", cursor + 1, msgCount, script[cursor].speaker.c_str(), script[cursor].text.c_str());

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                running = false;

            // 左键切换下一句
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                e.button.button == SDL_BUTTON_LEFT)
            {
                if (cursor + 1 < msgCount) {
                    cursor++;
                    // 释放旧文本贴图
                    SDL_DestroyTexture(speakerTex);
                    SDL_DestroyTexture(textTex);
                    speakerTex = createLineTexture(ren, font, script[cursor].speaker.c_str(), &speakerDst);
                    textTex = createLineTexture(ren, font, script[cursor].text.c_str(), &textDst);
                    printf("[%d/%d] %s: %s\n", cursor + 1, msgCount, script[cursor].speaker.c_str(), script[cursor].text.c_str());
                }
                // 最后一句停住
            }
        }

        SDL_SetRenderDrawColor(ren, 24, 26, 32, 255);
        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, speakerTex, nullptr, &speakerDst);
        SDL_RenderTexture(ren, textTex, nullptr, &textDst);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(speakerTex);
    SDL_DestroyTexture(textTex);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}