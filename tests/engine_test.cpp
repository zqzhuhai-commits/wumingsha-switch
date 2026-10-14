#include "engine.hpp"
#include "generated_data.hpp"
#include <cassert>
#include <iostream>
#include <set>
using namespace noname;
static Card c(Kind k,int suit=0){return {k,suit,7,900};}
static void integrity(const Game& g){
    assert(g.cardCount()==int(standardDeck.size()));std::set<int> ids;
    auto add=[&](Card x){assert(ids.insert(x.uid).second);};
    for(auto x:g.deck)add(x);
    for(auto x:g.discard)add(x);
    for(auto& p:g.players){for(auto x:p.hand)add(x);for(auto x:p.equipment)add(x);assert(p.hp<=p.maxHp);}
}
int main(){
    Game g;g.start(0,42);assert(g.players[0].hp==5);assert(g.players[0].hand.size()==6);integrity(g);
    assert(g.distance(0,2)==2);assert(g.distance(0,3)==1);
    g.players[1].alive=false;assert(g.distance(0,2)==1);
    assert(g.canAs(0,c(Kind::Tao,2),Kind::Sha));assert(!g.canAs(0,c(Kind::Tao,0),Kind::Sha));
    g.players[0].hero=2;assert(g.canAs(0,c(Kind::Sha),Kind::Shan));assert(g.canAs(0,c(Kind::Shan),Kind::Sha));
    g.start(1,2);g.players[0].hand={c(Kind::Sha)};g.players[0].shaUsed=4;assert(g.legal(0,0,1));
    g.players[0].hero=0;assert(!g.legal(0,0,1));g.players[0].equipment={c(Kind::Zhuge)};assert(g.legal(0,0,1));
    g.start(0,3);g.players[0].hand={c(Kind::Sha)};assert(!g.legal(0,0,2));assert(!g.legal(0,0,0));
    g.players[1].hand={c(Kind::Shan)};assert(g.play(0,1));assert(g.players[1].hp==4);assert(g.players[1].hand.empty());
    g.start(2,4);g.current=1;g.players[1].hand={c(Kind::Sha)};g.players[0].hand={c(Kind::Sha)};
    assert(g.play(0,0));assert(g.waiting);assert(!g.respond(0));assert(g.respond(0,true));assert(!g.waiting);assert(g.players[0].hp==5);
    g.start(0,5);g.players[0].hand={c(Kind::Juedou),c(Kind::Sha)};g.players[1].hand={c(Kind::Sha)};
    assert(g.play(0,1));assert(g.waiting);assert(g.respond(0));assert(g.players[1].hp==3);
    g.start(0,6);g.current=1;g.players[1].hand={c(Kind::Nanman)};g.players[0].hand={};
    assert(g.play(0,0));assert(g.waiting);g.passResponse();assert(g.players[0].hp==4);
    g.start(0,7);g.players[0].hand={c(Kind::Tao)};g.players[0].hp=1;g.current=1;g.players[1].hand={c(Kind::Sha)};
    assert(g.play(0,0));g.passResponse();assert(g.players[0].alive&&g.players[0].hp==1&&g.players[0].hand.empty());
    g.start(0,8);g.players[0].hand={c(Kind::Sha),c(Kind::Shan)};g.players[0].hp=1;
    assert(!g.endTurn()&&g.phase==Phase::Discard);assert(g.discardCard(0));assert(g.current==1);
    g.start(0,9);for(int p=1;p<4;++p)g.players[p].alive=false;g.checkVictory();assert(g.phase==Phase::Over&&g.result=="主公与忠臣获胜");
    g.start(0,10);g.players[0].alive=false;g.players[1].alive=false;g.players[2].alive=false;g.players[3].role=Role::Spy;g.checkVictory();assert(g.result=="内奸获胜");
    // Stress the real imported deck; assert unique ownership after every action.
    int finished=0;
    for(unsigned seed=0;seed<300;++seed){
        Game sim;sim.start(seed%4,seed);integrity(sim);
        for(int tick=0;tick<10000&&sim.phase!=Phase::Over;++tick){
            if(sim.waiting){bool used=false;for(int i=0;i<int(sim.players[0].hand.size());++i)if(sim.respond(i,true)){used=true;break;}if(!used)sim.passResponse();}
            else if(sim.current!=0)sim.aiStep();
            else if(sim.phase==Phase::Discard)sim.discardCard(0);
            else {bool used=false;for(int i=0;i<int(sim.players[0].hand.size())&&!used;++i)for(int t=1;t<4&&!used;++t){used=sim.play(i,t);if(!used)used=sim.play(i,t,true);}if(!used)sim.endTurn();}
            integrity(sim);
        }
        assert(sim.phase==Phase::Over);++finished;
    }
    std::cout<<"PASS: rule scenarios and "<<finished<<" complete games; "<<standardDeck.size()<<" unique cards conserved\n";
}
