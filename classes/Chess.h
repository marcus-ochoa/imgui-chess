#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
constexpr int negInfinite = -100000;
constexpr int posInfinite = 100000;

// Define constant bitmasks
constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); // A file mask
constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); // H file mask
constexpr uint64_t Rank3(0x0000000000FF0000ULL); // Rank 3 mask
constexpr uint64_t Rank6(0x0000FF0000000000ULL); // Rank 6 mask

// Player color constants
constexpr int WHITE = 0;
constexpr int BLACK = 1;

// enum ChessPiece
// {
//     NoPiece,
//     Pawn,
//     Knight,
//     Bishop,
//     Rook,
//     Queen,
//     King
// };

enum BitboardIndex
{
    WHITE_PAWNS,
    BLACK_PAWNS,
    WHITE_KNIGHTS,
    BLACK_KNIGHTS,
    WHITE_BISHOPS,
    BLACK_BISHOPS,
    WHITE_ROOKS,
    BLACK_ROOKS,
    WHITE_QUEENS,
    BLACK_QUEENS,
    WHITE_KING,
    BLACK_KING,
    WHITE_ALL_PIECES,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void clearBoardHighlights() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    void updateAI() override;
    bool gameHasAI() override { return true; }

    Grid* getGrid() override { return _grid; }

private:
    

    int _countMoves = 0;

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    std::vector<BitMove> generateAllMoves(std::string& state, int playerColor);
    BitboardElement generateKnightMoveBitboard(int square);
    BitboardElement generateKingMoveBitboard(int square);

    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, BitboardElement emptySquares);
    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, BitboardElement emptySquares);
    void generateBishopMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies);
    void generateQueenMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies);

    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnsBoard, BitboardElement emptySquares, BitboardElement enemyOccupancyBoard, int color);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitboardElement board, int shift);

    int negamax(std::string& state, int depth, int alpha, int beta, int playerColor);

    int evaluateBoard(const std::string& state);

    Grid* _grid;
    BitboardElement _knightBitboards[64];
    BitboardElement _kingBitboards[64];
    std::vector<BitMove> _moves;

    BitboardElement _bitboards[e_numBitboards];
    int _bitboardLookup[128];
};