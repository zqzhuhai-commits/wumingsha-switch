// Exercise touch hit regions and modal state using the actual native UI code.
#define main noname_application_main
#include "../source/main.cpp"
#undef main
#include <cassert>
#include <iostream>
int main(){
    assert(SDL_Init(SDL_INIT_TIMER)==0);
    App app;
    app.tap(405,275);assert(app.hero==1);
    app.tap(540,575);assert(!app.menu&&app.game.current==0);
    app.tap(610,200);assert(app.target==2);
    app.tap(1110,150);assert(app.target==2);
    app.tap(515,530);assert(app.selected==2);
    app.tap(535,455);assert(app.skill);
    app.tap(1150,35);assert(app.paused);
    app.tap(200,310);assert(app.paused);
    app.tap(540,310);assert(!app.paused);
    app.game.players[0].hand.resize(12,{Kind::Sha,0,7,900});
    app.selected=8;app.tap(400,530);assert(app.selected==9);
    app.action(Action::Pause);app.action(Action::End);assert(app.menu);
    SDL_Quit();std::cout<<"PASS: touch targets, card paging, skill toggle and pause controls\n";
}
