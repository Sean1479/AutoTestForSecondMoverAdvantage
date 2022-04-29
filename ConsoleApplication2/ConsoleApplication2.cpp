#include <cstdio>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <fstream>
#include <omp.h>
#include <cmath>

# define Xwin 0
# define Owin 1

using namespace std;
typedef pair<int, int> position;

class Node {
public:
    position pos;
    int total, score;
    int XO;
    Node* parent;//?
    vector<Node*> child;//?
    vector<pair<int, int>> valid_postion;
    Node(position p, int c, Node* par, vector<position> v);
};

class Board {
    int chess_board[8][8];
    position laststep;
public:
    Board();
    int lastXO;
    int sumscore();
    bool isend();
    bool search(position p, int player, int d);
    void flip(position p, int player, int d);
    void onemove(position p);
    vector<position> GetValidPosition(int, position);
    position random_move();
    position human_move();
    int simulate();
    void graph_board();
    void print_score();
};

class Tree {
    Node* root;
    Node* subroot, * tail;
    int nowchess;
public:
    Tree(int nchess, Board board);
    ~Tree();
    Node* expand(Board board);
    void nextnode(position nextpos, Board board);
    Node* best_child(Node* tarRoot, double cof);
    Board gettail(Board board);
    void backup(int result);
    void printinfo();
    void newturn();
};

int drow[8] = { 0,0,1,1,1,-1,-1,-1 };
int dcol[8] = { 1,-1,1,0,-1,1,0,-1 };

int win=0;
int lose=0;
int draw=0;

int main() {
    // modify Search time here, 1000 == 1 second
    int FirstMoverSearchTime = 1000;
    int SecondMoverSearchTime = 1000;
    srand(time(NULL));
    int x, y;
    time_t start, now;
    Board b = Board();
    Tree UCT(0, b);
    Node* best;
    int res, searchnum, total = 0;
    position rm;
    position hm;
    int searchtime = 500;
    int AIfirst = 1;
    int totalgame = 0;

    printf("=====init=====\n");
    while (totalgame < 5) {
        printf("=========totalgame:%d===========\n", totalgame);
        while (!b.isend()) {
            start = clock();
            now = clock();
            searchnum = 0;
            while ((int)(now - start) < FirstMoverSearchTime) {
                UCT.backup(UCT.gettail(b).simulate());
                now = clock();
                searchnum++;
            }
            now = clock();
            printf("Time use:%d  Search num:%d\n", (int)(now - start), searchnum);
            UCT.printinfo();
            best = UCT.best_child(NULL, 0);
            printf("win rate:%lf\n", 1.0 * best->score / best->total * 100); //!!
            b.onemove(best->pos);
            total++;
            printf("total chess:%d\n", total);
            b.graph_board();
            UCT.nextnode(best->pos, b);
            if (!b.isend()) {
                start = clock();
                now = clock();
                searchnum = 0;
                while ((int)(now - start) < SecondMoverSearchTime) {
                    UCT.backup(UCT.gettail(b).simulate());
                    now = clock();
                    searchnum++;
                }
                now = clock();
                printf("Time use:%d  Search num:%d\n", (int)(now - start), searchnum);
                UCT.printinfo();
                best = UCT.best_child(NULL, 0);
                printf("win rate:%lf\n", 1.0 * best->score / best->total * 100); //!!
                b.onemove(best->pos);
                total++;
                printf("total chess:%d\n", total);
                b.graph_board();
                UCT.nextnode(best->pos, b);
            }
        }
        printf("=========FINISH===========\n");
        b.print_score();
        totalgame++;
        printf("win=%d, lose=%d, draw=%d \n", win, lose, draw);
        printf("=========totalgame:%d===========\n", totalgame);
        total = 0;
        b = Board();
        UCT.newturn();
    }
}

Board::Board() {
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            chess_board[i][j] = -1;
    chess_board[3][3] = 0;
    chess_board[4][4] = 0;
    chess_board[3][4] = 1;
    chess_board[4][3] = 1;
    lastXO = 1;
    laststep = make_pair(-1, -1);
}

int Board::sumscore() {
    int mark[2] = { 0 };
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (chess_board[i][j] >= 0) mark[chess_board[i][j]]++;
    if (mark[0] == mark[1]) return 0;
    else if (mark[0] > mark[1]) return mark[0];
    else if (mark[0] < mark[1]) return -1 * mark[1];
}

bool Board::search(position pos, int XO, int d) {
    int row = pos.first + drow[d];
    int col = pos.second + dcol[d];
    if (chess_board[row][col] == XO) return false; // no same chess under this line

    while (0 <= row && row <= 7 && 0 <= col && col <= 7) {
        if (chess_board[row][col] == -1) return false;
        else if (chess_board[row][col] == XO) return true; //find same chess after different chess
        else {
            row += drow[d];
            col += dcol[d];
        }
    }
    return false; // after all search didnot find any then false
}

vector<position> Board::GetValidPosition(int nowXO = -1, position lstep = make_pair(-2, -2)) {
    vector<position> result;
    position pos;
    //?
    if (nowXO == -1) nowXO = !(bool)lastXO;
    if (lstep == make_pair(-2, -2)) lstep = laststep; //maybe useless

#pragma omp parallel for // OpenMP
    for (int n = 0; n < 64; n++) {
        int i = n / 8;
        int j = n % 8;
        if (chess_board[i][j] == -1) {
            pos = make_pair(i, j);
            for (int d = 0; d < 8; d++) {//direct
                if (search(pos, nowXO, d)) {
                    result.push_back(pos);
                    break;
                }
            }
        }
    }
    if (result.size() == 0) result.push_back(make_pair(-1, -1));
    return result;
}

bool Board::isend() {
    if (GetValidPosition(1, make_pair(-1, -1))[0].first == -1 && GetValidPosition(0, make_pair(-1, -1))[0].first == -1) return true;
    return false;
}

void Board::flip(position pos, int XO, int d) {
    int row = pos.first + drow[d];
    int col = pos.second + dcol[d];
    int oppositeXO = !(bool)XO;
    while (0 <= row && row <= 7 && 0 <= col && col <= 7 && chess_board[row][col] == oppositeXO) {
        chess_board[row][col] = XO;
        row += drow[d];
        col += dcol[d];
    }
}

void Board::onemove(position pos) {
    lastXO = !(bool)lastXO; // chess turn
    if (pos.first != -1) { // if legal
        chess_board[pos.first][pos.second] = lastXO;
        for (int d = 0; d < 8; d++) {
            if (search(pos, lastXO, d)) {
                flip(pos, lastXO, d);
            }
        }
    }
    laststep = pos; //change laststep value
}

position Board::random_move() {
    vector<pair<int, int>> validpos;
    int n;
    validpos = GetValidPosition();
    n = rand() % validpos.size();
    onemove(validpos[n]);
    return laststep;
}

position Board::human_move() {

    vector<pair<int, int>> validpos;
    position playerchoose;
    int px, py;
    validpos = GetValidPosition();
    if (validpos[0].first == -1) {
        printf("NO VALID POSITION CAN PUT\n");
        onemove(validpos[0]);
        return validpos[0];
    }
    else {
        while (true) {
            printf("Please choose where you want to put pieces.\nFor example type '1 1' if you want choose 11\nturn:  \n");
            scanf_s("%d%d", &py, &px);
            playerchoose = make_pair(py, px);
            for (auto vpos : validpos) {
                if (vpos == playerchoose) {
                    onemove(playerchoose);
                    return playerchoose;
                    break;
                }
            }
        }
    }
}
int Board::simulate() {
    int mark;
    int sXO = lastXO;
    int sBoard[8][8];
    position sstep = laststep;
    memcpy(sBoard, chess_board, sizeof(chess_board)); // store temporarily
    while (!isend()) {
        random_move();
    }
    mark = sumscore();
    memcpy(chess_board, sBoard, sizeof(chess_board)); // back
    laststep = sstep;
    lastXO = sXO;
    return mark;
}

void Board::graph_board() {
    int display[8][8];
    vector<position> vpos = GetValidPosition();
    memcpy(display, chess_board, sizeof(chess_board));
    if (laststep.first != -1) display[laststep.first][laststep.second] = 2;
    for (auto pos : vpos) display[pos.first][pos.second] = 3;
    printf("    0   1   2   3   4   5   6   7\n");
    for (int i = 0; i < 8; i++) {
        printf(" %d", i);
        for (int j = 0; j < 8; j++) {
            switch (display[i][j]) {
            case 0: printf("| X "); break;
            case 1: printf("| O "); break;
            case 2: {
                if (lastXO == 0) printf("| X!");
                else printf("| O!");
                break; }
            case 3: printf("| ? "); break;
            case -1: printf("|   "); break;
            }
        }
        printf("|\n");
    }
    printf("===================================\n");
}

void Board::print_score() {
    int mark[2] = { 0 };
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (chess_board[i][j] != -1) mark[chess_board[i][j]]++;
    printf("X=%d : O=%d \n", mark[0], mark[1]);
     if (mark[0]>mark[1]){
         win++;
     }
     else if(mark[0]<mark[1]){
         lose++;
     }
     else{
         draw++;
     }
}

Node::Node(position p, int c, Node* par, vector<position> posv) {
    pos = p;
    XO = c;
    parent = par;
    total = 0;
    score = 0;
    valid_postion = posv;
}

Tree::Tree(int nchess, Board board) {
    root = new Node(make_pair(-1, -1), 1, NULL, board.GetValidPosition());
    subroot = root;
    tail = root;
    nowchess = nchess;
}

Tree::~Tree()
{

}

Node* Tree::expand(Board board) {
    Node* newNode;
    vector<position> possiblepos;
    bool suitable;
    for (auto vpos : tail->valid_postion) {
        suitable = false;
        for (auto cpos : tail->child) {
            if (vpos == cpos->pos) {
                suitable = true;
                break;
            }
        }
        if (!suitable) possiblepos.push_back(vpos);
    }
    int rp = rand() % possiblepos.size();
    board.onemove(possiblepos[rp]);
    newNode = new Node(possiblepos[rp], !(bool)tail->XO, tail, board.GetValidPosition());
    tail->child.push_back(newNode);
    return newNode;

}

void Tree::nextnode(position nextpos, Board board) {
    for (auto c : subroot->child) {
        if (c->pos == nextpos) {
            subroot = c;
            subroot->score = 0;
            subroot->total = 0;
            subroot->child.clear();
            return;
        }
    }
    // if no child suitable
    board.onemove(nextpos);
    Node* newNode = new Node(nextpos, !(bool)subroot->XO, subroot, board.GetValidPosition());
    subroot = newNode;
}

Node* Tree::best_child(Node* tarroot = NULL, double R = 1.41) {
    double maxresult = -99999999;
    double UCB;
    Node* bestnode = NULL;
    if (tarroot == NULL) tarroot = subroot;
    for (auto cpos : tarroot->child) {
        UCB = 1.0 * cpos->score / cpos->total + R * sqrt(log(tarroot->total) / cpos->total);
        if (UCB > maxresult) {
            maxresult = UCB;
            bestnode = cpos;
        }
    }
    // printf("bestnode ok");
    // printf("\nargmax%d=ucb%d\n",maxresult,UCB);
    return bestnode;
}

Board Tree::gettail(Board board) {
    tail = subroot;
    while (!board.isend()) {
        int vpos = tail->valid_postion.size();
        int cpos = tail->child.size();
        if (vpos != cpos) {
            tail = expand(board);
            board.onemove(tail->pos);
            break;
        }
        else {
            tail = best_child(tail);
            board.onemove(tail->pos);
        }
    }
    return board;
}

void Tree::backup(int result) {
    int who_win = result > 0 ? Xwin : Owin;
    result = abs(result);
    while (tail != subroot) {
        tail->total += 1;
        if (!(tail->XO ^ who_win)) tail->score += 1;
        tail = tail->parent;
    }
    tail->total += 1;
}

void Tree::printinfo() {
    int i = 0;
    printf("subroot:(%d,%d)\n", subroot->pos.first, subroot->pos.second);
    // printf("childnum%d",sizeof(subroot->child));
    for (auto c : subroot->child) {
        i++;
        printf("%d", i);
        printf("-child:(%d,%d), win:%d, total:%d\n", c->pos.first, c->pos.second, c->score, c->total);
    }
    Node* n = best_child(subroot, 0);
    position p = n == NULL ? make_pair(8, 8) : n->pos;
    printf("=====bestchild:(%d,%d)=====\n", p.first, p.second);
}

void Tree::newturn() {
    subroot = root;
}
