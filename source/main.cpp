// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.hpp"
#include "generated_data.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <chrono>
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
std::map<int,TTF_Font*> fonts;
std::map<std::string,SDL_Texture*> texts;
std::string assets;
void box(int x,int y,int w,int h,SDL_Color col){SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void border(int x,int y,int w,int h,SDL_Color col){SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);}
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
void button(const std::string& s,int x,int y,int w,bool enabled=true){box(x,y,w,48,enabled?SDL_Color{56,76,67,255}:SDL_Color{29,40,39,255});border(x,y,w,48,enabled?gold:muted);text(s,x+16,y+8,22,enabled?cream:muted,w-25);}
enum class Action { None, Left, Right, Up, Down, Confirm, Cancel, End, Skill, Pause, Exit };
struct App {
    Game game;
    bool menu=true,paused=false,quit=false,skill=false,help=false;
    int hero=0,selected=0,target=1;
    Uint32 nextAi=0;
    Uint32 toastUntil=0;
    std::string toast;
    void start(){game.start(hero,unsigned(std::chrono::system_clock::now().time_since_epoch().count()));menu=false;paused=false;selected=0;target=1;skill=false;nextAi=SDL_GetTicks()+700;}
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
        if(paused){if(y>=290&&y<338)action(Action::Confirm);else if(y>=355&&y<403)action(Action::End);else if(y>=420&&y<468)action(Action::Exit);return;}
        if(menu){if(y>=225&&y<495&&x>=60&&x<1220){hero=std::min(3,(x-60)/300);}if(y>=550&&y<598)action(Action::Confirm);
        if(y>=625)help=true;
            return;}
        if(game.phase==Phase::Over){action(Action::Confirm);return;}
        if(y<65&&x>1080){action(Action::Pause);return;}
        if(y>=100&&y<=305){if(x>=35&&x<=375)target=1;
        if(x>=405&&x<=745)target=2;
        if(x>=775&&x<=1115)target=3;
            return;}
        if(y>=450&&y<620&&x>=290&&x<=1230){int first=(selected/8)*8;int count=std::min(8,int(game.players[0].hand.size())-first);
        if(count>0){int idx=std::min(count-1,(x-290)/110);selected=first+idx;}return;}
        if(y>=644&&y<692){if(x>=290&&x<485)action(Action::Confirm);else if(x>=500&&x<695)action(Action::Skill);else if(x>=710&&x<905)action(Action::End);else if(x>=920&&x<1230)action(Action::Cancel);}
    }
    void tick(){if(menu||paused||help||game.phase==Phase::Over)return;
        if(SDL_GetTicks()>=nextAi&&game.current!=0&&!game.waiting){game.aiStep();normalize();nextAi=SDL_GetTicks()+550;}}
    void background(){
        for(int y=0;y<720;++y)box(0,y,1280,1,SDL_Color{Uint8(12+y/100),Uint8(30+y/75),Uint8(28+y/100),255});
        border(18,18,1244,684,SDL_Color{53,69,58,255});
        text("无名杀",38,24,32,gold);text("NATIVE SWITCH  /  原生移植体验版 0.1",175,34,18,muted);
        box(35,74,1210,1,SDL_Color{81,89,64,255});
    }
    void panel(int p,int x,int y,int w){
        const auto& q=game.players[p];bool active=game.current==p;
        box(x,y,w,202,q.alive?SDL_Color{31,51,46,255}:SDL_Color{26,31,31,255});
        border(x,y,w,202,p==target?gold:SDL_Color{64,87,75,255});
        image(q.hero,x+12,y+12,106,153);
        if(!q.alive)box(x+12,y+12,106,153,SDL_Color{10,10,10,190});
        text(game.name(p),x+134,y+15,30,q.alive?cream:muted);
        const bool reveal=p==0||!q.alive||game.phase==Phase::Over;
        text(reveal?Game::roleName(q.role):"身份未知",x+134,y+60,20,reveal?gold:muted);
        text("体力 "+std::to_string(std::max(0,q.hp))+" / "+std::to_string(q.maxHp),x+134,y+99,22,q.hp<=1?red:cream);
        text("手牌 "+std::to_string(q.hand.size())+(!q.equipment.empty()?" · 连弩":""),x+134,y+135,20,muted);
        text(active?"正在行动":(!q.alive?"已阵亡":(p==target?"当前目标":"等待回合")),x+12,y+175,18,active?gold:muted);
    }
    void render(){
        background();
        if(menu){
            text("选择你的武将",60,112,44,cream);text("四人身份局 / 单机 AI / 你担任主公",62,173,23,muted);
            for(int i=0;i<4;++i){int x=60+i*300;box(x,225,260,270,{31,50,44,255});border(x,225,260,270,i==hero?gold:muted);image(i,x+10,235,134,194);text(heroNames[i],x+157,251,30,gold,85);text(skills[i],x+15,451,19,cream,230);
        if(i==3)text("反间暂未移植",x+149,325,16,muted,100);}
            button("A  开始对局",480,550,320);text("方向键选将 · Y  玩法说明 · −  退出",418,627,23,muted);text("原生精简版：11种牌 / 4名武将 / 无扩展与联机",411,663,18,muted);
        }else{
            text("回合 "+std::to_string(game.turns)+"  ·  牌堆 "+std::to_string(game.deck.size())+"  ·  弃牌 "+std::to_string(game.discard.size()),610,34,20,muted);
            text("+ 暂停",1128,30,23,gold);
            panel(1,35,100,340);panel(2,405,100,340);panel(3,775,100,340);
            box(35,328,230,349,{31,49,44,255});image(game.players[0].hero,51,344,78,112);text(game.name(0),143,352,28,gold);text("你 · 主公",143,396,18,cream);
            text("体力 "+std::to_string(std::max(0,game.players[0].hp))+" / "+std::to_string(game.players[0].maxHp),50,470,25,cream);
            text(skills[game.players[0].hero],50,519,19,gold,204);text("↑↓ 或 L/R 选目标",50,566,18,muted,205);text("←→ 选择手牌",50,602,18,muted);text("救援自动使用桃",50,638,18,muted);
            auto& h=game.players[0].hand;
            std::string status;
            if(game.waiting)status="需要打出【"+std::string(cardNames[int(game.effects.front().response)])+"】  ·  A 响应 / B 放弃";
            else if(game.phase==Phase::Discard)status=game.current==0?"弃牌阶段：A 弃置所选手牌，直到手牌不超过体力":"AI 正在弃牌";
            else if(game.current==0)status="出牌阶段：选择手牌和目标后按 A";
            else status=game.players[0].alive?"AI 正在行动，请等待":"你已阵亡 · 继续观战";
            text(SDL_GetTicks()<toastUntil?toast:status,290,321,24,gold,860);
            int row=0;
    for(auto& entry:game.log){text(entry,290+(row/3)*445,360+(row%3)*24,17,muted,425);++row;
        if(row==6)break;}
            if(!h.empty()){
                int first=(selected/8)*8;
                for(int i=first;i<int(h.size())&&i<first+8;++i){const Card c=h[i];int x=290+(i-first)*110,y=i==selected?448:461;
                    box(x,y,102,156,i==selected?SDL_Color{232,217,175,255}:SDL_Color{206,202,180,255});border(x,y,102,156,gold);
                    SDL_Color ink=c.red()?SDL_Color{148,47,39,255}:SDL_Color{38,45,41,255};
                    const char* suits[]={"黑桃","梅花","红桃","方块"};text(std::string(suits[c.suit])+" "+std::to_string(c.rank),x+9,y+10,17,ink,85);
                    std::string cn=cardNames[int(c.kind)];text(cn,x+9,y+60,c.kind<=Kind::Tao?40:20,ink,85);text(std::to_string(i+1),x+10,y+128,14,ink);
                    if(i==selected)text(skill?"技能":"选中",x+56,y+128,14,ink,42);
                }
                text(std::to_string(selected+1)+" / "+std::to_string(h.size())+"  ·  "+tips[int(h[selected].kind)],290,616,18,muted,940);
            }else text("暂无手牌",540,514,32,muted);
            button(game.phase==Phase::Discard?"A  弃置":"A  使用 / 响应",290,644,195);
            button(skill?"Y  技能已开启":"Y  技能转换",500,644,195);
            button("X  结束回合",710,644,195,game.current==0&&!game.waiting);
            button(game.waiting?"B  放弃响应":"B  取消技能",920,644,310);
            if(game.phase==Phase::Over){box(330,190,620,320,{13,28,26,248});border(330,190,620,320,gold);text("对局结束",515,233,42,gold);text(game.result,455,305,36,cream,430);button("A  返回选将",480,411,320);}
        }
        if(paused){box(330,185,620,330,{13,28,26,250});border(330,185,620,330,gold);text("暂停",583,225,36,gold);button("A  继续对局",480,290,320);button("X  返回选将",480,355,320);button("−  退出程序",480,420,320);}
        if(help){box(115,100,1050,550,{13,28,26,252});border(115,100,1050,550,gold);text("玩法说明",155,130,36,gold);
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
    SDL_Window* window=SDL_CreateWindow("无名杀 Switch Native 0.1",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,0);
    if(!window)return 5;
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer){SDL_DestroyWindow(window);return 6;}
    SDL_RenderSetLogicalSize(renderer,1280,720);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    TTF_Font* probe=TTF_OpenFont((assets+"font.ttf").c_str(),24);
    if(!probe){std::fprintf(stderr,"Font unavailable: %s\n",TTF_GetError());return 7;}fonts[24]=probe;
    for(int i=0;i<4;++i)portraits[i]=IMG_LoadTexture(renderer,(assets+"portraits/"+heroIds[i]+".jpg").c_str());
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
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);IMG_Quit();TTF_Quit();SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
    return 0;
}
