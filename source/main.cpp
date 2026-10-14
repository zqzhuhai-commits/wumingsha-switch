// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.hpp"
#include "generated_data.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#ifdef __SWITCH__
#include <switch.h>
#endif
using namespace noname;
namespace {
constexpr SDL_Color cream{234,229,206,255},gold{206,170,95,255}, muted{153,172,168,255}, red{208,88,70,255};
constexpr const char* skills[]={"武圣 · 红牌可当杀","咆哮 · 杀无次数限制","龙胆 · 杀闪互相转换","英姿 · 摸牌阶段多摸一张"};
constexpr const char* tips[]={"造成1点伤害 · 距离1 · 每回合1次","抵消杀 · 在响应阶段打出","回复1点体力 · 濒死自动救援","摸两张牌","弃置目标一张牌 · 优先装备","距离1 · 获得目标一张牌","双方轮流打出杀 · 首次由目标响应","其他角色打出杀，否则受到1点伤害","其他角色打出闪，否则受到1点伤害","所有存活角色回复1点体力","装备后使用杀无次数限制 · 距离仍为1"};
SDL_Renderer* renderer=nullptr;
SDL_Texture* portraits[4]={};
SDL_Texture* tableTexture=nullptr;
SDL_Texture* cardTextures[11]={};
TTF_Font* suitFont=nullptr;
SDL_Texture* suitTextures[4]={};
std::map<int,TTF_Font*> fonts;
std::map<std::string,SDL_Texture*> texts;
std::string assets;
void box(int x,int y,int w,int h,SDL_Color col){SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void border(int x,int y,int w,int h,SDL_Color col){SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);}
void texture(SDL_Texture* t,int x,int y,int w,int h){if(!t)return;SDL_Rect dst{x,y,w,h};SDL_RenderCopy(renderer,t,nullptr,&dst);}
void orb(int x,int y,int radius,SDL_Color col){
    SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
    for(int dy=-radius;dy<=radius;++dy){int dx=int(std::sqrt(double(radius*radius-dy*dy)));SDL_RenderDrawLine(renderer,x-dx,y+dy,x+dx,y+dy);}
}
void suit(int index,int x,int y,SDL_Color color){
    if(!suitFont)return;
    const char* values[]={"♠","♣","♥","♦"};
    if(!suitTextures[index]){
        SDL_Surface* surface=TTF_RenderUTF8_Blended(suitFont,values[index],color);
        if(!surface)return;
        suitTextures[index]=SDL_CreateTextureFromSurface(renderer,surface);
        SDL_FreeSurface(surface);
    }
    texture(suitTextures[index],x,y,20,20);
}
void text(const std::string& value,int x,int y,int size=24,SDL_Color color=cream,int maxw=0){
    if(value.empty())return;
    auto it=fonts.find(size);
        if(it==fonts.end()){TTF_Font* f=TTF_OpenFont((assets+"font.ttf").c_str(),size);
        if(!f)return;
        it=fonts.emplace(size,f).first;}
    std::string key=std::to_string(size)+":"+std::to_string(color.r)+","+std::to_string(color.g)+","+std::to_string(color.b)+":"+value;
    auto t=texts.find(key);
    if(t==texts.end()){
        if(texts.size()>400){for(auto& entry:texts)SDL_DestroyTexture(entry.second);texts.clear();}
        SDL_Surface* s=TTF_RenderUTF8_Blended(it->second,value.c_str(),color);
        if(!s)return;
        SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,s);SDL_FreeSurface(s);
        if(!texture)return;
        t=texts.emplace(key,texture).first;
    }
    int w,h;SDL_QueryTexture(t->second,nullptr,nullptr,&w,&h);
    if(maxw>0 && w>maxw){h=h*maxw/w;w=maxw;}
    SDL_Rect dst{x,y,w,h};SDL_RenderCopy(renderer,t->second,nullptr,&dst);
}
void image(int hero,int x,int y,int w,int h){if(!portraits[hero])return;SDL_Rect r{x,y,w,h};SDL_RenderCopy(renderer,portraits[hero],nullptr,&r);}
void button(const std::string& s,int x,int y,int w,bool enabled=true){
    box(x+3,y+4,w,48,{21,17,14,170});
    box(x,y,w,48,enabled?SDL_Color{39,30,21,245}:SDL_Color{91,83,70,245});
    border(x,y,w,48,enabled?gold:muted);border(x+4,y+4,w-8,40,{109,89,60,255});
    text(s,x+16,y+8,23,enabled?SDL_Color{239,212,149,255}:muted,w-25);
}
enum class Action { None, Left, Right, Up, Down, Confirm, Cancel, End, Skill, Pause, Exit };
struct App {
    Game game;
    bool menu=true,paused=false,quit=false,skill=false,help=false;
    int hero=0,selected=0,target=1;
    Uint32 nextAi=0;
    Uint32 startedAt=0;
    Uint32 toastUntil=0;
    std::string toast;
    void start(){game.start(hero,unsigned(std::chrono::system_clock::now().time_since_epoch().count()));menu=false;paused=false;selected=0;target=1;skill=false;startedAt=SDL_GetTicks();nextAi=SDL_GetTicks()+700;}
    void normalize(){auto& h=game.players[0].hand;selected=h.empty()?0:std::min(selected,int(h.size()-1));
        if(!game.players[target].alive)cycleTarget(1);}
    void cycleTarget(int direction){for(int i=0;i<3;++i){target=1+(target-1+direction+3)%3;
        if(game.players[target].alive)return;}}
    void action(Action a){
        if(a==Action::None)return;
        if(help){if(a==Action::Confirm||a==Action::Cancel||a==Action::Pause)help=false;return;}
        if(menu){
            if(a==Action::Left)hero=(hero+3)%4;
        if(a==Action::Right)hero=(hero+1)%4;
            if(a==Action::Confirm)start();
        if(a==Action::Skill)help=true;
        if(a==Action::Exit)quit=true;
            return;
        }
        if(paused){if(a==Action::Pause||a==Action::Confirm||a==Action::Cancel)paused=false;
        if(a==Action::End){menu=true;paused=false;}if(a==Action::Exit)quit=true;
        if(a==Action::Skill)help=true;
            return;}
        if(a==Action::Pause){paused=true;return;}
        if(game.phase==Phase::Over){if(a==Action::Confirm){menu=true;}return;}
        auto& h=game.players[0].hand;
        if(a==Action::Left&&!h.empty())selected=(selected+int(h.size())-1)%int(h.size());
        if(a==Action::Right&&!h.empty())selected=(selected+1)%int(h.size());
        if(a==Action::Up)cycleTarget(-1);
        if(a==Action::Down)cycleTarget(1);
        if(a==Action::Skill)skill=!skill;
        if(a==Action::Cancel){if(game.waiting)game.passResponse();else skill=false;}
        if(a==Action::End && game.current==0 && !game.waiting){game.endTurn();skill=false;}
        if(a==Action::Confirm){
            bool accepted=false;
            if(game.waiting)accepted=game.respond(selected,skill);
            else if(game.current==0){if(game.phase==Phase::Discard)accepted=game.discardCard(selected);else accepted=game.play(selected,target,skill);}
            if(!accepted){toast="无法使用：检查牌型、目标、距离、杀次数；技能转换需按 Y";toastUntil=SDL_GetTicks()+2500;}
            skill=false;
        }
        normalize();nextAi=SDL_GetTicks()+600;
    }
    void tap(int x,int y){
        if(help){help=false;return;}
        if(paused){
            if(x<480||x>800)return;
            if(y>=290&&y<338)action(Action::Confirm);
            else if(y>=355&&y<403)action(Action::End);
            else if(y>=420&&y<468)action(Action::Exit);
            return;
        }
        if(menu){
            if(y>=225&&y<495&&x>=60&&x<1220)hero=std::min(3,(x-60)/300);
            if(y>=550&&y<598&&x>=480&&x<=800)action(Action::Confirm);
            if(y>=625)help=true;
            return;
        }
        if(game.phase==Phase::Over){action(Action::Confirm);return;}
        if(y<65&&x>1100){action(Action::Pause);return;}
        if(y>=100&&y<=355){
            for(int i=1;i<=3;++i){int px=290+(i-1)*280;if(x>=px&&x<=px+190&&game.players[i].alive){target=i;return;}}
        }
        if(y>=480&&y<668&&x>=270&&x<=1180){
            int first=(selected/8)*8;
            int count=std::min(8,int(game.players[0].hand.size())-first);
            if(count>0)selected=first+std::min(count-1,(x-270)/115);
            return;
        }
        if(y>=434&&y<482){
            if(x>=290&&x<485)action(Action::Confirm);
            else if(x>=500&&x<695)action(Action::Skill);
            else if(x>=710&&x<905)action(Action::End);
            else if(x>=920&&x<1230)action(Action::Cancel);
        }
    }
    void tick(){if(menu||paused||help||game.phase==Phase::Over)return;
        if(SDL_GetTicks()>=nextAi&&game.current!=0&&!game.waiting){game.aiStep();normalize();nextAi=SDL_GetTicks()+550;}}
    void background(){
        if(tableTexture)texture(tableTexture,0,0,1280,720);
        else for(int y=0;y<720;++y)box(0,y,1280,1,{Uint8(84+y/45),Uint8(69+y/60),Uint8(52+y/85),255});
        box(0,0,1280,74,{31,23,17,190});
        border(12,12,1256,696,{141,115,72,255});
        border(16,16,1248,688,{54,40,27,255});
        text("无名杀 Switch版",32,24,30,{240,213,152,255});
        text("原生体验版 0.1.1",274,34,18,cream);
    }
    void panel(int p,int x,int y,int w,int h=255){
        const auto& q=game.players[p];
        bool active=game.current==p;
        SDL_Color frame=active?SDL_Color{150,194,106,255}:(p==target?gold:SDL_Color{99,88,70,255});
        box(x+5,y+5,w,h,{17,13,11,165});
        box(x,y,w,h,{39,34,27,255});
        image(q.hero,x+27,y+6,w-33,h-38);
        box(x+5,y+5,24,h-10,q.hero==3?SDL_Color{59,91,55,235}:SDL_Color{126,65,39,235});
        text(q.hero==3?"吴":"蜀",x+6,y+9,23,{249,229,161,255});
        std::string n=game.name(p);
        text(n.substr(0,3),x+6,y+40,21,cream);
        text(n.substr(3),x+6,y+64,21,cream);
        for(int i=0;i<q.maxHp;++i){int yy=y+h-42-i*18;
            orb(x+17,yy,7,i<q.hp?SDL_Color{35,132,61,255}:SDL_Color{93,87,73,255});
            if(i<q.hp)orb(x+15,yy-2,3,{145,218,125,255});
        }
        box(x+30,y+h-35,w-35,30,{35,25,18,218});
        const bool reveal=p==0||!q.alive||game.phase==Phase::Over;
        text(reveal?Game::roleName(q.role):"身份未知",x+39,y+h-31,17,reveal?gold:cream);
        text(std::to_string(q.hand.size())+"张",x+w-55,y+h-31,17,cream);
        orb(x+w-19,y+22,14,{216,209,177,235});
        text(reveal?"明":"?",x+w-28,y+10,23,{102,72,36,255},21);
        if(!q.alive){box(x+29,y+6,w-35,h-43,{15,13,12,195});text("阵亡",x+55,y+h/2-18,35,red,w-65);}
        if(!q.equipment.empty()){box(x+31,y+h-64,w-38,26,{191,178,137,230});text("诸葛连弩",x+41,y+h-60,16,{48,37,28,255});}
        border(x,y,w,h,frame);border(x+2,y+2,w-4,h-4,frame);
        if(active)text("行动中",x+w/2-30,y-24,17,{224,233,174,255});
        else if(p==target)text("当前目标",x+w/2-36,y-24,17,{251,222,155,255});
    }
    void render(){
        background();
        if(menu){
            text("选择你的武将",60,112,44,cream);text("四人身份局 / 单机 AI / 你担任主公",62,173,23,muted);
            for(int i=0;i<4;++i){int x=60+i*300;box(x,225,260,270,{41,32,25,215});border(x,225,260,270,i==hero?gold:muted);image(i,x+10,235,134,194);text(heroNames[i],x+157,251,30,gold,85);text(skills[i],x+15,451,19,cream,230);
        if(i==3)text("反间暂未移植",x+149,325,16,muted,100);}
            button("A  开始对局",480,550,320);text("方向键选将 · Y  玩法说明 · −  退出",418,627,23,muted);text("原生精简版：11种牌 / 4名武将 / 无扩展与联机",411,663,18,muted);
        }else{
            int elapsed=int((SDL_GetTicks()-startedAt)/1000);
            char clock[32];std::snprintf(clock,sizeof(clock),"%02d:%02d",elapsed/60,elapsed%60);
            text(std::string(clock)+"  ·  第"+std::to_string(game.turns)+"回合",750,32,23,cream);
            button("+ 暂停",1120,20,115);
            text("主×1  忠×1  反×1  内×1",32,105,19,{246,222,155,255},225);
            text("四人身份局",32,141,25,cream);
            text("你的身份：主公",32,184,22,gold);
            text("击败反贼与内奸",32,219,21,cream);
            text("牌堆 "+std::to_string(game.deck.size()),32,278,23,cream);
            text("弃牌 "+std::to_string(game.discard.size()),32,311,20,cream);
            panel(1,290,100,190);panel(2,570,100,190);panel(3,850,100,190);
            panel(0,35,425,205,248);
            text("你 · "+std::string(skills[game.players[0].hero]),35,682,17,{245,221,160,255},220);
            text("↑↓ / L R 选目标",1090,130,17,cream,150);
            text("←→ 选择手牌",1090,162,17,cream,150);
            text("桃自动救援",1090,194,17,cream,150);
            auto& h=game.players[0].hand;
            std::string status;
            if(game.waiting)status="请打出【"+std::string(cardNames[int(game.effects.front().response)])+"】 · A 响应 / B 放弃";
            else if(game.phase==Phase::Discard)status=game.current==0?"弃牌阶段：按 A 弃牌，直到手牌不超过体力":"AI 正在弃牌";
            else if(game.current==0)status="请选择要使用的牌";
            else status=game.players[0].alive?"AI 正在行动，请等待":"你已阵亡 · 继续观战";
            box(270,390,960,36,{40,30,22,170});
            text(SDL_GetTicks()<toastUntil?toast:status,290,397,23,{244,224,163,255},915);
            if(!game.log.empty())text(game.log.back(),320,371,17,{238,229,207,255},750);
            button(game.phase==Phase::Discard?"A  弃置":"A  确定 / 响应",290,434,195);
            button(skill?"Y  技能已开启":"Y  技能转换",500,434,195);
            button("X  结束回合",710,434,195,game.current==0&&!game.waiting);
            button(game.waiting?"B  放弃响应":"B  取消技能",920,434,310);
            if(!h.empty()){
                int first=(selected/8)*8;
                for(int i=first;i<int(h.size())&&i<first+8;++i){
                    const Card c=h[i];int x=270+(i-first)*115,y=i==selected?492:503;
                    box(x+3,y+4,108,166,{20,15,10,170});
                    if(cardTextures[int(c.kind)])texture(cardTextures[int(c.kind)],x,y,108,166);
                    else box(x,y,108,166,{218,206,174,255});
                    if(i==selected){border(x-2,y-2,112,170,{255,216,116,255});border(x-3,y-3,114,172,gold);}
                    SDL_Color ink=c.red()?SDL_Color{151,31,25,255}:SDL_Color{35,33,28,255};
                    box(x+3,y+3,28,41,{234,225,196,245});
                    const char* ranks[]={"","A","2","3","4","5","6","7","8","9","10","J","Q","K"};
                    text(ranks[c.rank],x+7,y+3,20,ink,25);suit(c.suit,x+5,y+23,ink);
                    if(i==selected)text(skill?"技能":"选中",x+62,y+144,16,{70,43,18,255},42);
                }
                text(std::to_string(selected+1)+" / "+std::to_string(h.size())+"  ·  "+tips[int(h[selected].kind)],275,681,17,{242,229,195,255},945);
            }else text("暂无手牌",650,557,32,cream);
            if(game.phase==Phase::Over){box(330,190,620,320,{39,28,20,248});border(330,190,620,320,gold);text("对局结束",515,233,42,gold);text(game.result,455,305,36,cream,430);button("A  返回选将",480,411,320);}
        }
        if(paused){box(330,185,620,330,{35,26,19,250});border(330,185,620,330,gold);text("暂停",583,225,36,gold);button("A  继续对局",480,290,320);button("X  返回选将",480,355,320);button("−  退出程序",480,420,320);}
        if(help){box(115,100,1050,550,{35,26,19,252});border(115,100,1050,550,gold);text("玩法说明",155,130,36,gold);
            const char* lines[]={"你是主公；击败反贼和内奸获胜。忠臣协助主公，内奸需要最后独胜。","每回合摸2张牌，出牌后弃到手牌数不超过体力。主公体力上限加1。","杀：距离1，每回合1次；闪：响应杀；桃：治疗自己或自动救援。","关羽红牌当杀；张飞不限杀次数；赵云杀闪转换；周瑜额外摸1张。","Y 开启转换，再按 A 使用或响应。红色的桃也会被关羽转换成杀。","顺手/过河优先操作装备，否则随机操作手牌；连弩解除杀次数限制。","本版简化：救援自动结算；不含反间、无懈、其他装备、判定和扩展。","A 或 B 关闭说明。对局中按 + 暂停，暂停后按 − 退出。"};
            for(int i=0;i<8;++i)text(lines[i],155,201+i*48,23,i==6?gold:cream,965);
        }
    }
};
#ifndef __SWITCH__
Action keyboard(SDL_Keycode k){switch(k){case SDLK_LEFT:return Action::Left;case SDLK_RIGHT:return Action::Right;case SDLK_UP:return Action::Up;case SDLK_DOWN:return Action::Down;case SDLK_RETURN:case SDLK_a:return Action::Confirm;case SDLK_b:case SDLK_BACKSPACE:return Action::Cancel;case SDLK_x:return Action::End;case SDLK_y:return Action::Skill;case SDLK_ESCAPE:case SDLK_p:return Action::Pause;case SDLK_MINUS:return Action::Exit;default:return Action::None;}}
#endif
}
int main(int argc,char** argv){
#ifdef __SWITCH__
    if(R_FAILED(romfsInit()))return 1;
    assets="romfs:/";
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState pad;padInitializeDefault(&pad);hidInitializeTouchScreen();
#else
    assets="romfs/";
#endif
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)return 2;
    if(TTF_Init()!=0){SDL_Quit();return 3;}
    if(!(IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG)&IMG_INIT_JPG)){TTF_Quit();SDL_Quit();return 4;}
    SDL_Window* window=SDL_CreateWindow("无名杀 Switch版 0.1.1",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,0);
    if(!window)return 5;
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer){SDL_DestroyWindow(window);return 6;}
    SDL_RenderSetLogicalSize(renderer,1280,720);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    TTF_Font* probe=TTF_OpenFont((assets+"font.ttf").c_str(),24);
    if(!probe){std::fprintf(stderr,"Font unavailable: %s\n",TTF_GetError());return 7;}fonts[24]=probe;
    for(int i=0;i<4;++i)portraits[i]=IMG_LoadTexture(renderer,(assets+"portraits/"+heroIds[i]+".jpg").c_str());
    tableTexture=IMG_LoadTexture(renderer,(assets+"ui/table.jpg").c_str());
    suitFont=TTF_OpenFont((assets+"ui/suits.ttf").c_str(),24);
    const char* cardIds[]={"sha","shan","tao","wuzhong","guohe","shunshou","juedou","nanman","wanjian","taoyuan","zhuge"};
    for(int i=0;i<11;++i)cardTextures[i]=IMG_LoadTexture(renderer,(assets+"ui/cards/"+cardIds[i]+".png").c_str());
    App app;
#ifndef __SWITCH__
    std::string screenshot;
    for(int i=1;i<argc;++i){if(std::string(argv[i])=="--table")app.start();else if(std::string(argv[i])=="--screenshot"&&i+1<argc)screenshot=argv[++i];}
#else
    (void)argc;(void)argv;
    bool wasTouch=false;
#endif
    while(!app.quit){
#ifdef __SWITCH__
        if(!appletMainLoop())break;
        padUpdate(&pad);u64 down=padGetButtonsDown(&pad);
        if(down&HidNpadButton_Left)app.action(Action::Left);
        if(down&HidNpadButton_Right)app.action(Action::Right);
        if(down&(HidNpadButton_Up|HidNpadButton_L))app.action(Action::Up);
        if(down&(HidNpadButton_Down|HidNpadButton_R))app.action(Action::Down);
        if(down&HidNpadButton_A)app.action(Action::Confirm);
        if(down&HidNpadButton_B)app.action(Action::Cancel);
        if(down&HidNpadButton_X)app.action(Action::End);
        if(down&HidNpadButton_Y)app.action(Action::Skill);
        if(down&HidNpadButton_Plus)app.action(Action::Pause);
        if(down&HidNpadButton_Minus)app.action(Action::Exit);
        HidTouchScreenState touch{};hidGetTouchScreenStates(&touch,1);
        if(touch.count&&!wasTouch)app.tap(touch.touches[0].x,touch.touches[0].y);
        wasTouch=touch.count>0;
#endif
        SDL_Event e;while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT)app.quit=true;
#ifndef __SWITCH__
            if(e.type==SDL_KEYDOWN)app.action(keyboard(e.key.keysym.sym));
            if(e.type==SDL_MOUSEBUTTONDOWN){int ww,hh;SDL_GetWindowSize(window,&ww,&hh);app.tap(e.button.x*1280/ww,e.button.y*720/hh);}
#endif
        }
        app.tick();app.render();
#ifndef __SWITCH__
        if(!screenshot.empty()){SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_RGBA32);SDL_RenderReadPixels(renderer,nullptr,s->format->format,s->pixels,s->pitch);int rc=IMG_SavePNG(s,screenshot.c_str());SDL_FreeSurface(s);
        if(rc)std::fprintf(stderr,"Screenshot failed: %s\n",IMG_GetError());
        app.quit=true;}
#endif
        SDL_RenderPresent(renderer);SDL_Delay(16);
    }
    for(auto& t:texts)SDL_DestroyTexture(t.second);
    for(auto& f:fonts)TTF_CloseFont(f.second);
    for(auto t:portraits)if(t)SDL_DestroyTexture(t);
    for(auto t:cardTextures)if(t)SDL_DestroyTexture(t);
    if(tableTexture)SDL_DestroyTexture(tableTexture);
    if(suitFont)TTF_CloseFont(suitFont);
    for(auto t:suitTextures)if(t)SDL_DestroyTexture(t);
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);IMG_Quit();TTF_Quit();SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
    return 0;
}
