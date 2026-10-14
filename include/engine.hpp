// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace noname {
enum class Kind { Sha, Shan, Tao, Wuzhong, Guohe, Shunshou, Juedou, Nanman, Wanjian, Taoyuan, Zhuge };
struct Card { Kind kind; int suit, rank, uid; bool red() const { return suit >= 2; } };
enum class Role { Lord, Loyal, Rebel, Spy };
enum class Phase { Play, Discard, Over };
struct Player {
    int hero=0, hp=4, maxHp=4, shaUsed=0;
    Role role=Role::Rebel;
    bool alive=true;
    std::vector<Card> hand;
    std::vector<Card> equipment; // Only Zhuge is supported in v0.1.
};
struct Effect { int source, target; Kind response; bool duel=false; };
class Game {
public:
    std::array<Player,4> players;
    std::vector<Card> deck, discard;
    std::deque<Effect> effects;
    std::deque<std::string> log;
    Phase phase=Phase::Play;
    int current=0, turns=0;
    bool waiting=false;
    std::string result;
    explicit Game(unsigned seed=1) : rng(seed) {}
    void start(int hero, unsigned seed);
    void draw(int player, int count);
    bool canAs(int player, const Card& card, Kind kind) const;
    int distance(int a, int b) const;
    bool legal(int player, int card, int target, bool skill=false) const;
    bool play(int card, int target, bool skill=false);
    bool respond(int card, bool skill=false);
    void passResponse();
    bool endTurn();
    bool discardCard(int card);
    void aiStep();
    void settle();
    void checkVictory();
    int cardCount() const;
    std::string name(int i) const;
    static const char* roleName(Role role);
private:
    std::mt19937 rng;
    void message(const std::string& text);
    Card remove(int p, int index, bool toDiscard=true);
    int responseIndex(int p, Kind kind) const;
    int enemy(int p) const;
    bool friendly(int a,int b) const;
    void damage(int target,int source);
    void nextTurn();
    void beginTurn();
};
}
