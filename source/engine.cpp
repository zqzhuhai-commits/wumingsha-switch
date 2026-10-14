// SPDX-License-Identifier: GPL-3.0-or-later
// Native subset of noname rules. The upstream JS/DOM engine is not embedded.
#include "engine.hpp"
#include "generated_data.hpp"
#include <limits>

namespace noname {
const char* Game::roleName(Role r) {
    switch(r) { case Role::Lord:return "主公";case Role::Loyal:return "忠臣";case Role::Rebel:return "反贼";case Role::Spy:return "内奸"; }
    return "未知";
}
std::string Game::name(int i) const { return heroNames[players[i].hero]; }
void Game::message(const std::string& s) { log.push_back(s); while(log.size()>7)log.pop_front(); }
void Game::start(int hero,unsigned seed) {
    rng.seed(seed); players={}; deck.assign(standardDeck.begin(),standardDeck.end()); discard.clear();
    effects.clear();log.clear();phase=Phase::Play;waiting=false;result.clear();current=0;turns=0;
    std::shuffle(deck.begin(),deck.end(),rng);
    std::array<Role,3> roles={Role::Rebel,Role::Loyal,Role::Spy};std::shuffle(roles.begin(),roles.end(),rng);
    for(int i=0;i<4;++i) {
        auto& p=players[i];p.hero=(hero+i)%4;p.role=i==0?Role::Lord:roles[i-1];
        p.maxHp=heroHp[p.hero]+(i==0?1:0);p.hp=p.maxHp;draw(i,4);
    }
    message("四人身份局 · 你是主公 · 救援自动结算");beginTurn();
}
void Game::draw(int p,int count) {
    for(int i=0;i<count;++i) {
        if(deck.empty()) { deck.swap(discard);std::shuffle(deck.begin(),deck.end(),rng); }
        if(deck.empty())return;
        players[p].hand.push_back(deck.back());deck.pop_back();
    }
}
Card Game::remove(int p,int i,bool toDiscard) {
    Card c=players[p].hand.at(i);players[p].hand.erase(players[p].hand.begin()+i);
    if(toDiscard)discard.push_back(c);
    return c;
}
bool Game::canAs(int p,const Card& c,Kind kind) const {
    if(c.kind==kind)return true;
    if(players[p].hero==0 && kind==Kind::Sha && c.red())return true; // 武圣
    if(players[p].hero==2 && ((kind==Kind::Sha && c.kind==Kind::Shan)||(kind==Kind::Shan && c.kind==Kind::Sha)))return true; // 龙胆
    return false;
}
int Game::responseIndex(int p,Kind k) const {
    for(size_t i=0;i<players[p].hand.size();++i)if(players[p].hand[i].kind==k)return int(i);
    for(size_t i=0;i<players[p].hand.size();++i)if(canAs(p,players[p].hand[i],k))return int(i);
    return -1;
}
int Game::distance(int a,int b) const {
    if(a==b)return 0;
    int cw=0,ccw=0;
    for(int i=(a+1)%4;;i=(i+1)%4){ if(players[i].alive)++cw;if(i==b)break; }
    for(int i=(a+3)%4;;i=(i+3)%4){ if(players[i].alive)++ccw;if(i==b)break; }
    return std::min(cw,ccw);
}
bool Game::legal(int p,int i,int t,bool skill) const {
    if(phase!=Phase::Play||waiting||!effects.empty()||p!=current||!players[p].alive||i<0||i>=int(players[p].hand.size()))return false;
    const Card c=players[p].hand[i];Kind k=c.kind;
    if(skill){if(!canAs(p,c,Kind::Sha))return false;k=Kind::Sha;}
    const bool targeted=k==Kind::Sha||k==Kind::Juedou||k==Kind::Guohe||k==Kind::Shunshou;
    if(targeted && (t<0||t>=4||t==p||!players[t].alive))return false;
    switch(k) {
    case Kind::Sha:return distance(p,t)<=1&&(players[p].shaUsed==0||players[p].hero==1||!players[p].equipment.empty());
    case Kind::Shan:return false;
    case Kind::Tao:return players[p].hp<players[p].maxHp;
    case Kind::Shunshou:return distance(p,t)<=1&&(!players[t].hand.empty()||!players[t].equipment.empty());
    case Kind::Guohe:return !players[t].hand.empty()||!players[t].equipment.empty();
    default:return true;
    }
}
bool Game::play(int i,int t,bool skill) {
    int p=current;if(!legal(p,i,t,skill))return false;
    const Card c=remove(p,i);const Kind k=skill?Kind::Sha:c.kind;
    message(name(p)+" 使用 "+(skill?"技能·杀":cardNames[int(k)]));
    switch(k) {
    case Kind::Sha:++players[p].shaUsed;effects.push_back({p,t,Kind::Shan,false});break;
    case Kind::Tao:++players[p].hp;break;
    case Kind::Wuzhong:draw(p,2);break;
    case Kind::Guohe:case Kind::Shunshou:{
        Card taken;
        if(!players[t].equipment.empty()) { taken=players[t].equipment.back();players[t].equipment.pop_back(); }
        else {int n=int(rng()%players[t].hand.size());taken=remove(t,n,false);}
        if(k==Kind::Shunshou)players[p].hand.push_back(taken);else discard.push_back(taken);
        break;
    }
    case Kind::Juedou:effects.push_back({p,t,Kind::Sha,true});break;
    case Kind::Nanman:case Kind::Wanjian:
        for(int n=1;n<4;++n){int q=(p+n)%4;if(players[q].alive)effects.push_back({p,q,k==Kind::Nanman?Kind::Sha:Kind::Shan,false});}break;
    case Kind::Taoyuan:for(auto& q:players)if(q.alive)q.hp=std::min(q.hp+1,q.maxHp);break;
    case Kind::Zhuge:
        discard.pop_back(); // Move the physical card from discard to equipment.
        for(auto old:players[p].equipment)discard.push_back(old);
        players[p].equipment={c};break;
    case Kind::Shan:break;
    }
    settle();return true;
}
bool Game::friendly(int a,int b) const {
    if(a==b)return true;
    // Do not use a hidden identity to decide the human lord's rescue policy.
    return players[a].role==Role::Loyal && players[b].role==Role::Lord;
}
void Game::damage(int t,int s) {
    auto& p=players[t];--p.hp;message(name(t)+" 受到1点伤害");
    // v0.1: rescue is automatic, using actual Peach cards only.
    for(int n=0;n<4 && p.hp<=0;++n) {
        int q=(t+n)%4;if(!players[q].alive||!friendly(q,t))continue;
        for(int i=0;i<int(players[q].hand.size()) && p.hp<=0;)
            if(players[q].hand[i].kind==Kind::Tao){remove(q,i);++p.hp;message(name(q)+" 使用桃救援 "+name(t));}else ++i;
    }
    if(p.hp>0)return;
    p.alive=false;message(name(t)+" 阵亡 · "+roleName(p.role));
    for(auto c:p.hand)discard.push_back(c);
    p.hand.clear();
    for(auto c:p.equipment)discard.push_back(c);
    p.equipment.clear();
    checkVictory();
    if(phase==Phase::Over)return;
    if(players[s].alive && p.role==Role::Rebel)draw(s,3);
    if(players[s].alive && players[s].role==Role::Lord && p.role==Role::Loyal) {
        for(auto c:players[s].hand)discard.push_back(c);
    players[s].hand.clear();
        for(auto c:players[s].equipment)discard.push_back(c);
    players[s].equipment.clear();
        message("主公击杀忠臣：弃置全部手牌和装备");
    }
}
void Game::checkVictory() {
    int alive=0;bool rebels=false,spy=false;
    for(auto& p:players)if(p.alive){++alive;rebels|=p.role==Role::Rebel;spy|=p.role==Role::Spy;}
    if(!players[0].alive){result=alive==1&&spy?"内奸获胜":"反贼获胜";phase=Phase::Over;}
    else if(!rebels&&!spy){result="主公与忠臣获胜";phase=Phase::Over;}
    if(phase==Phase::Over){waiting=false;effects.clear();message(result);}
}
void Game::settle() {
    waiting=false;
    while(!effects.empty() && phase!=Phase::Over) {
        auto e=effects.front();
        if(!players[e.target].alive){effects.pop_front();continue;}
        if(e.target==0){waiting=true;return;}
        int i=responseIndex(e.target,e.response);effects.pop_front();
        if(i>=0){remove(e.target,i);message(name(e.target)+" 打出 "+cardNames[int(e.response)]);
            if(e.duel && players[e.source].alive)effects.push_front({e.target,e.source,Kind::Sha,true});
        } else damage(e.target,e.source);
    }
    if(phase!=Phase::Over && !players[current].alive)nextTurn();
}
bool Game::respond(int i,bool skill) {
    if(!waiting||effects.empty()||i<0||i>=int(players[0].hand.size()))return false;
    auto e=effects.front();auto c=players[0].hand[i];
    if(c.kind!=e.response && !(skill && canAs(0,c,e.response)))return false;
    remove(0,i);effects.pop_front();message(name(0)+" 打出 "+cardNames[int(e.response)]);
    if(e.duel&&players[e.source].alive)effects.push_front({0,e.source,Kind::Sha,true});
    settle();return true;
}
void Game::passResponse(){if(!waiting||effects.empty())return;auto e=effects.front();effects.pop_front();damage(e.target,e.source);settle();}
void Game::beginTurn(){
    ++turns;phase=Phase::Play;players[current].shaUsed=0;
    draw(current,players[current].hero==3?3:2);message(name(current)+" 的回合 · 摸牌");
}
void Game::nextTurn(){
    if(phase==Phase::Over)return;
    for(int n=0;n<4;++n){current=(current+1)%4;if(players[current].alive){beginTurn();return;}}
}
bool Game::endTurn(){
    if(phase==Phase::Over||waiting||!effects.empty())return false;
    if(int(players[current].hand.size())>players[current].hp){phase=Phase::Discard;return false;}
    nextTurn();return true;
}
bool Game::discardCard(int i){
    if(phase!=Phase::Discard||i<0||i>=int(players[current].hand.size()))return false;
    remove(current,i);if(int(players[current].hand.size())<=players[current].hp)nextTurn();return true;
}
int Game::enemy(int p) const {
    if(players[p].role==Role::Rebel && players[0].alive)return 0;
    int chosen=-1,best=-1000;
    // AI uses only public roles; unrevealed hand cards and identities are not inspected here.
    for(int q=0;q<4;++q)if(q!=p&&players[q].alive) {
        if(players[p].role==Role::Loyal && q==0)continue;
        int score=(players[p].role==Role::Spy?players[q].hp:10-players[q].hp);
        if(players[p].role==Role::Spy && q==0 && std::count_if(players.begin(),players.end(),[](const Player& x){return x.alive;})>2)score-=20;
        if(distance(p,q)==1)score+=2;
        if(score>best){best=score;chosen=q;}
    }
    return chosen;
}
void Game::aiStep(){
    if(phase==Phase::Over||waiting||current==0)return;
    int p=current;auto& h=players[p].hand;
    if(phase==Phase::Discard){
        int chosen=0,best=100;
        for(int i=0;i<int(h.size());++i){int val=h[i].kind==Kind::Tao?20:h[i].kind==Kind::Shan?15:5;
            if(val<best){best=val;chosen=i;}}
        discardCard(chosen);return;
    }
    for(int i=0;i<int(h.size());++i)if(h[i].kind==Kind::Tao||h[i].kind==Kind::Wuzhong||h[i].kind==Kind::Zhuge)
        if(legal(p,i,p)){play(i,p);return;}
    int t=enemy(p);
    for(int i=0;i<int(h.size());++i) {
        Kind k=h[i].kind;
        if(k==Kind::Shan||k==Kind::Tao||k==Kind::Taoyuan)continue;
        if(legal(p,i,t)){play(i,t);return;}
        if(canAs(p,h[i],Kind::Sha)&&legal(p,i,t,true)){play(i,t,true);return;}
    }
    endTurn();
}
int Game::cardCount() const {
    int n=int(deck.size()+discard.size());for(const auto& p:players)n+=int(p.hand.size()+p.equipment.size());return n;
}
}
