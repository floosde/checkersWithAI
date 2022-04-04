#include <iostream>

//~~~~ board.h

#include <random>
#include <memory>
#include <utility>
#include <vector>
#include <queue>
#include <set>
#include <limits>

enum class Side {
    NoOne,
    Bot,
    User,
};

class Piece {
public:
    Side side;
    std::pair<int, int> position;
    bool is_king;

    Piece() : side(Side::NoOne), is_king(false) {
    }
};

class Move {
public:
    Piece *piece;
    std::vector<Piece *> pieces_eaten;
    std::pair<int, int> final_position;
    bool is_final;
    bool fake = false;

    explicit Move(Piece *new_piece, std::pair<int, int> new_position, bool new_is_final) : piece(
            new_piece),
                                                                                           pieces_eaten(
                                                                                                   std::vector<Piece *>()),
                                                                                           final_position(
                                                                                                   std::move(
                                                                                                           new_position)),
                                                                                           is_final(
                                                                                                   new_is_final) {

    }

    explicit Move(Piece *new_piece, std::pair<int, int> new_position, bool new_is_final, bool new_fake) : piece(
            new_piece),
                                                                                                          pieces_eaten(
                                                                                                                  std::vector<Piece *>()),
                                                                                                          final_position(
                                                                                                                  std::move(
                                                                                                                          new_position)),
                                                                                                          is_final(
                                                                                                                  new_is_final),
                                                                                                          fake(new_fake) {
    }
};

class Board {
public:
    using BoardType = std::vector<std::vector<Piece>>;
    std::vector<Piece *> bot;
    std::vector<Piece *> user;
    size_t bot_kings;
    size_t user_kings;
    BoardType board;

    Board() : board(BoardType(8, std::vector<Piece>(8))),
              bot_kings(0),
              user_kings(0) {
        for (size_t i = 0; i < 8; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                if (i < 3 && (i + j) % 2 == 0) {
                    board[i][j].position = std::make_pair(i, j);
                    board[i][j].side = Side::Bot;
                    bot.push_back(&board[i][j]);
                } else if (i >= 5 && (i + j) % 2 == 0) {
                    board[i][j].position = std::make_pair(i, j);
                    board[i][j].side = Side::User;
                    user.push_back(&board[i][j]);
                } else {
                    board[i][j].position = std::make_pair(i, j);
                    board[i][j].side = Side::NoOne;
                }
            }
        }
    }

    Board(Board &other) : board(BoardType(8, std::vector<Piece>(8))),
                          bot_kings(other.bot_kings),
                          user_kings(other.user_kings) {
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                board[i][j].side = other.board[i][j].side;
                board[i][j].position = other.board[i][j].position;
                board[i][j].is_king = other.board[i][j].is_king;
            }
        }

        for (Piece *piece: other.user) {
            user.push_back(&board[piece->position.first][piece->position.second]);
        }

        for (Piece *piece: other.bot) {
            bot.push_back(&board[piece->position.first][piece->position.second]);
        }
    }

    void DeletePiece(Piece *piece) {
        if (piece->side == Side::User) {
            auto elem = std::find(user.begin(), user.end(), piece);
            if (piece->is_king) {
                --user_kings;
            }
            user.erase(elem);
        } else {
            auto elem = std::find(bot.begin(), bot.end(), piece);
            if (piece->is_king) {
                --bot_kings;
            }
            bot.erase(elem);
        }
        piece->side = Side::NoOne;
        piece->is_king = false;
    }

    static bool AllFinal(const std::deque<Move> &moves) {
        for (const Move &move: moves) {
            if (!move.is_final) {
                return false;
            }
        }
        return true;
    }

    static bool AlreadyEaten(const std::vector<Piece *> &eaten, int i, int j) {
        for (Piece *piece: eaten) {
            if (piece->position.first == i && piece->position.second == j) {
                return true;
            }
        }
        return false;
    }

    std::deque<Move> CurrentMoves(Side side) {
        std::deque<Move> moves;
        std::vector<Piece *> &curr_player = side == Side::Bot ? bot : user;
        bool go_up = side == Side::User;
        Side opponent = side == Side::User ? Side::Bot : Side::User;

        for (Piece *piece: curr_player) {
            auto temp_move = Move(piece, piece->position, false, true);
            auto curr_piece_moves = CalculatePossibleMoves(temp_move, opponent, go_up);
            for (const Move &move: curr_piece_moves) {
                moves.push_back(move);
            }
        }

        if (!AllFinal(moves)) {
            for (auto it = moves.begin(); it != moves.end();) {
                if (it->is_final) {
                    it = moves.erase(it);
                } else {
                    ++it;
                }
            }
        }

        while (!AllFinal(moves)) {
            Move curr_move = moves.back();
            if (curr_move.is_final) {
                moves.push_front(curr_move);
                moves.pop_back();
                continue;
            }
            moves.pop_back();
            auto new_moves = CalculatePossibleMoves(curr_move, opponent, go_up);
            for (const Move &new_move: new_moves) {
                if (new_move.is_final) {
                    moves.push_front(new_move);
                } else {
                    moves.push_back(new_move);
                }
            }
        }
        return moves;
    }

    std::vector<Move> CalculatePossibleMoves(const Move &curr_move,
                                             Side opponent,
                                             bool go_up) {
        auto[i, j] = curr_move.final_position;
        std::vector<Move> moves;
        bool has_to_fight = false;
        if ((i + 1 < 8 && j + 1 < 8 && i + 2 < 8 && j + 2 < 8)
            && board[i + 1][j + 1].side == opponent
            && board[i + 2][j + 2].side == Side::NoOne
            && (!go_up
                || curr_move.piece->is_king
                || !curr_move.pieces_eaten.empty())
            && !AlreadyEaten(curr_move.pieces_eaten, i + 1, j + 1)) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i + 2, j + 2);
            moves.back().pieces_eaten.push_back(&board[i + 1][j + 1]);
            moves.back().is_final = false;
            has_to_fight = true;
        }

        if ((i - 1 >= 0 && j + 1 < 8 && i - 2 >= 0 && j + 2 < 8)
            && board[i - 1][j + 1].side == opponent
            && board[i - 2][j + 2].side == Side::NoOne
            && (go_up
                || curr_move.piece->is_king
                || !curr_move.pieces_eaten.empty())
            && !AlreadyEaten(curr_move.pieces_eaten, i - 1, j + 1)) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i - 2, j + 2);
            moves.back().pieces_eaten.push_back(&board[i - 1][j + 1]);
            moves.back().is_final = false;
            has_to_fight = true;
        }

        if ((i + 1 < 8 && j - 1 >= 0 && i + 2 < 8 && j - 2 >= 0)
            && board[i + 1][j - 1].side == opponent
            && board[i + 2][j - 2].side == Side::NoOne
            && (!go_up
                || curr_move.piece->is_king
                || !curr_move.pieces_eaten.empty())
            && !AlreadyEaten(curr_move.pieces_eaten, i + 1, j - 1)) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i + 2, j - 2);
            moves.back().pieces_eaten.push_back(&board[i + 1][j - 1]);
            moves.back().is_final = false;
            has_to_fight = true;
        }

        if ((i - 1 >= 0 && j - 1 >= 0 && i - 2 >= 0 && j - 2 >= 0)
            && board[i - 1][j - 1].side == opponent
            && board[i - 2][j - 2].side == Side::NoOne
            && (go_up
                || curr_move.piece->is_king
                || !curr_move.pieces_eaten.empty())
            && !AlreadyEaten(curr_move.pieces_eaten, i - 1, j - 1)) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i - 2, j - 2);
            moves.back().pieces_eaten.push_back(&board[i - 1][j - 1]);
            moves.back().is_final = false;
            has_to_fight = true;
        }

        if (has_to_fight) {
            return moves;
        }

        if ((i - 1 >= 0 && j + 1 < 8 && (go_up || curr_move.piece->is_king))
            && board[i - 1][j + 1].side == Side::NoOne && curr_move.pieces_eaten.empty()) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i - 1, j + 1);
            moves.back().is_final = true;
        }

        if ((i - 1 >= 0 && j - 1 >= 0 && (go_up || curr_move.piece->is_king))
            && board[i - 1][j - 1].side == Side::NoOne && curr_move.pieces_eaten.empty()) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i - 1, j - 1);
            moves.back().is_final = true;
        }

        if ((i + 1 < 8 && j + 1 < 8 && (!go_up || curr_move.piece->is_king))
            && board[i + 1][j + 1].side == Side::NoOne && curr_move.pieces_eaten.empty()) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i + 1, j + 1);
            moves.back().is_final = true;
        }

        if ((i + 1 < 8 && j - 1 >= 0 && (!go_up || curr_move.piece->is_king))
            && board[i + 1][j - 1].side == Side::NoOne && curr_move.pieces_eaten.empty()) {
            moves.push_back(curr_move);
            moves.back().fake = false;
            moves.back().final_position = std::make_pair(i + 1, j - 1);
            moves.back().is_final = true;
        }

        if (moves.empty() && !curr_move.fake) {
            moves.push_back(curr_move);
            moves.back().is_final = true;
        }
        return moves;
    }

    void ImplementMove(Move *move) {
        auto[i_from, j_from] = move->piece->position;
        auto[i_to, j_to] = move->final_position;
        Piece &curr_piece = board[i_from][j_from];
        board[i_to][j_to].side = move->piece->side;
        board[i_to][j_to].is_king = curr_piece.is_king;
        board[i_to][j_to].position = std::make_pair(i_to, j_to);
        curr_piece.side = Side::NoOne;
        curr_piece.is_king = false;
        for (Piece *eaten: move->pieces_eaten) {
            DeletePiece(eaten);
        }
        if (board[i_to][j_to].side == Side::User) {
            auto elem = std::find(user.begin(), user.end(), move->piece);
            user.erase(elem);
            user.push_back(&board[move->final_position.first][move->final_position.second]);
            user.back()->position = std::make_pair(i_to, j_to);
            if (user.back()->position.first == 0) {
                user.back()->is_king = true;
                ++user_kings;
            }
        } else {
            auto elem = std::find(bot.begin(), bot.end(), move->piece);
            bot.erase(elem);
            bot.push_back(&board[move->final_position.first][move->final_position.second]);
            bot.back()->position = std::make_pair(i_to, j_to);
            if (bot.back()->position.first == 7) {
                bot.back()->is_king = true;
                ++bot_kings;
            }
        }
    }

    int Score() const {
        return int(bot.size() - user.size() + (bot_kings - user_kings) / 2);
    }

    static int SimulateMoveAndScore(const Board &board, const Move &move) {
        size_t opponent_kings =
                move.piece->side == Side::User ? board.bot_kings : board.user_kings; //add kings counter to board
        int eaten_opponent_kings = 0;
        for (auto &eaten_piece: move.pieces_eaten) {
            if (eaten_piece->is_king) {
                ++eaten_opponent_kings;
            }
        }
        return int(board.Score() + move.pieces_eaten.size() + (opponent_kings - eaten_opponent_kings) / 2);
    }

    static Move CopyMove(Board &new_board, Move &move_to_copy) {
        auto[i_pos, j_pos] = move_to_copy.piece->position;
        Move new_move(&new_board.board[i_pos][j_pos], move_to_copy.final_position, move_to_copy.is_final,
                      move_to_copy.fake);
        for (auto &piece: move_to_copy.pieces_eaten) {
            auto[i_eaten, j_eaten] = piece->position;
            new_move.pieces_eaten.push_back(&new_board.board[i_eaten][j_eaten]);
        }
        return new_move;
    }

    static int MinMaxAI(Board &prev_board, Move &curr_move, int alpha, int beta, int depth, Side side) {
        auto &this_side_pieces = side == Side::User ? prev_board.user : prev_board.bot;
        if (depth == 0 || this_side_pieces.empty()) {
            return SimulateMoveAndScore(prev_board, curr_move);
        }
        auto board = Board(prev_board);
        auto move_copy = CopyMove(board, curr_move);
        board.ImplementMove(&move_copy);

        if (side == Side::User) {
            int max_score = std::numeric_limits<int>::min();
            auto moves = board.CurrentMoves(Side::Bot);
            sort(moves.begin(), moves.end(), [&](const Move &lhs, const Move &rhs) {
                return SimulateMoveAndScore(board, lhs) > SimulateMoveAndScore(board, rhs);
            });
            if (moves.empty()) {
                return SimulateMoveAndScore(prev_board, curr_move);
            }
            for (Move &move: moves) {
                int curr_score = MinMaxAI(board, move, alpha, beta, depth - 1, Side::Bot);
                max_score = std::max(curr_score, max_score);
                alpha = std::max(alpha, curr_score);
                if (beta <= alpha) {
                    break;
                }
            }
            return max_score;
        } else {
            int min_score = std::numeric_limits<int>::max();
            auto moves = board.CurrentMoves(Side::User);
            sort(moves.begin(), moves.end(), [&](const Move &lhs, const Move &rhs) {
                return SimulateMoveAndScore(board, lhs) < SimulateMoveAndScore(board, rhs);
            });
            if (moves.empty()) {
                return SimulateMoveAndScore(prev_board, curr_move);
            }
            for (Move &move: moves) {
                int curr_score = MinMaxAI(board, move, alpha, beta, depth - 1, Side::User);
                min_score = std::min(curr_score, min_score);
                beta = std::min(beta, curr_score);
                if (beta <= alpha) {
                    break;
                }
            }
            return min_score;
        }
    }

    bool BotMove() {
        auto moves = CurrentMoves(Side::Bot);
        sort(moves.begin(), moves.end(), [&](const Move &lhs, const Move &rhs) {
            return SimulateMoveAndScore(*this, lhs) > SimulateMoveAndScore(*this, rhs);
        });

        if (moves.empty()) {
            return false;
        }

        std::cout << "Possible moves:\n";
        for (Move &move: moves) {
            std::cout << "From (" << move.piece->position.first << ", " << move.piece->position.second << ")"
                      << " to (" << move.final_position.first << ", " << move.final_position.second << ")\n";
        }

        int score = std::numeric_limits<int>::min();
        Move *chosen_move = nullptr;
        if (moves.size() != 1) {
            for (Move &move: moves) {
                int curr_score = MinMaxAI(*this, move,
                                          std::numeric_limits<int>::min(),
                                          std::numeric_limits<int>::max(),
                                          12,
                                          Side::Bot);
                if (curr_score > score) {
                    chosen_move = &move;
                    score = curr_score;
                }
            }
        } else {
            chosen_move = &moves[0];
        }

        std::cout << "Made "
                  << "(" << chosen_move->piece->position.first << ", " << chosen_move->piece->position.second << ") to "
                  << "(" << chosen_move->final_position.first << ", " << chosen_move->final_position.second << ")\n";

        if (!chosen_move->pieces_eaten.empty()) {
            std::cout << "Pieces eaten: ";
            for (auto &eaten_piece: chosen_move->pieces_eaten) {
                std::cout << "(" << eaten_piece->position.first << ", " << eaten_piece->position.second << ") ";
            }
            std::cout << "\n";
        }

        ImplementMove(chosen_move);
        return true;
    }

    bool PlayerMove() {
        auto moves = CurrentMoves(Side::User);

        if (moves.empty()) {
            return false;
        }

        std::cout << "Possible moves:\n";
        for (Move &move: moves) {
            std::cout << "From (" << move.piece->position.first << ", " << move.piece->position.second << ")"
                      << " to (" << move.final_position.first << ", " << move.final_position.second << ")\n";
        }

        Move *chosen_move = nullptr;
        while (chosen_move == nullptr) {
            std::string from, to;
            std::cin >> from;
            std::cin >> to;
            int i_from = from[0] - 48;
            int j_from = from[1] - 48;
            int i_to = to[0] - 48;
            int j_to = to[1] - 48;
            for (Move &move: moves) {
                if (move.piece->position == std::make_pair(i_from, j_from)
                    && move.final_position == std::make_pair(i_to, j_to)) {
                    chosen_move = &move;
                    break;
                }
            }
            if (chosen_move == nullptr) {
                std::cout << "Try again!\n";
            }
        }

        std::cout << "Made "
                  << "(" << chosen_move->piece->position.first << ", " << chosen_move->piece->position.second << ") to "
                  << "(" << chosen_move->final_position.first << ", " << chosen_move->final_position.second << ")\n";

        if (!chosen_move->pieces_eaten.empty()) {
            std::cout << "Pieces eaten: ";
            for (auto &eaten_piece: chosen_move->pieces_eaten) {
                std::cout << "(" << eaten_piece->position.first << ", " << eaten_piece->position.second << ") ";
            }
            std::cout << "\n";
        }

        ImplementMove(chosen_move);
        return true;
    }
};

//~~~~

// class Game
class Game {
public:
    Board new_board;

    explicit Game() : new_board(Board()) {
    }

    void PlayGame() {
        while (true) {
            if (!new_board.PlayerMove()) {
                std::cout << "AI Win!\n";
                break;
            }
            PrintBoard();
            if (!new_board.BotMove()) {
                std::cout << "Player Win!\n";
                break;
            }
            PrintBoard();
        }
    }

    void PrintBoard() {
        std::cout << "  ";
        for (int i = 0; i < 8; ++i) {
            std::cout << i << " ";
        }
        std::cout << "\n";
        for (int i = 0; i < 8; ++i) {
            std::cout << i << " ";
            for (int j = 0; j < 8; ++j) {
                if (new_board.board[i][j].side == Side::NoOne) {
                    std::cout << "~ ";
                }

                if (new_board.board[i][j].side == Side::Bot) {
                    std::cout << (new_board.board[i][j].is_king ? "◓ " : "○ ");
                }

                if (new_board.board[i][j].side == Side::User) {
                    std::cout << (new_board.board[i][j].is_king ? "◒ " : "● ");
                }
            }
            std::cout << "\n";
        }
    }
};

//

int main() {
    Game game;
    game.PrintBoard();
    game.PlayGame();
    return 0;
}
