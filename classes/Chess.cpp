#include "Chess.h"
#include <limits>
#include <cmath>
#include <map>
#include <regex>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    for (int i = 0; i < 64; i++) {
        _knightBitboards[i] = generateKnightMoveBitboard(i);
        _kingBitboards[i] = generateKingMoveBitboard(i);
    }
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    int tag = (playerNumber == 0) ? piece : piece + 128;
    bit->setGameTag(tag);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    // FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    _moves = generateAllMoves();
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    std::map<char, ChessPiece> pieceTypes {
        {'p', ChessPiece::Pawn},
        {'n', ChessPiece::Knight},
        {'b', ChessPiece::Bishop},
        {'r', ChessPiece::Rook},
        {'q', ChessPiece::Queen},
        {'k', ChessPiece::King},
    };

    // I GOT THIS REGEX FROM CHATGPT (SEEMS TO CHECK OUT, ALSO CHECKED ON REGEX101)
    // Regex explanation:
    // 1. ([rnbqkpRNBQKP1-8/]+) → piece placement
    // 2. (?:\s+([wb]))? → optional active color
    // 3. (?:\s+([-KQkq]+))? → optional castling availability
    // 4. (?:\s+([-a-h1-8]+))? → optional en passant target square
    // 5. (?:\s+(\d+))? → optional halfmove clock
    // 6. (?:\s+(\d+))? → optional fullmove number
    std::regex fenRegex(
        "^\\s*([rnbqkpRNBQKP1-8/]+)(?:\\s+([wb]))?(?:\\s+([-KQkq]+))?(?:\\s+([-a-h1-8]+))?(?:\\s+(\\d+))?(?:\\s+(\\d+))?\\s*$"
    );
    std::smatch match;

    std::string fields[6];

    // If there is a match, place the strings in the corresponding fields
    if (std::regex_match(fen, match, fenRegex)) {
        for (size_t i = 1; i < match.size(); i++) {
            if (match[i].matched) {
                fields[i - 1] = match[i].str();
            }
            else (fields[i - 1] = "-");
        }
    }
    else return;

    if (fields[0] != "-") {
        int row = 7;
        int col = 0;
        for (char ch : fields[0]) {
            if (isdigit(ch)) {
                col += ch - '0'; // If you see a number, move over that number of columns
            }
            else if (ch == '/') {
                row--; // Go up a row when you see a forward slash
                col = 0;
            }
            else {
                int playerNum = isupper(ch) ? 0 : 1; // Uppercase pieces are white, lowercase are black
                Bit* bit = PieceForPlayer(playerNum, pieceTypes[tolower(ch)]);
                if (bit) {
                    ChessSquare *square = _grid->getSquare(col, row);
                    bit->setPosition(square->getPosition());
                    square->setBit(bit);
                }

                col++;
            }
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    bool ret = false;
    ChessSquare* square = (ChessSquare *) &src;

    if (square) {
        int squareIndex = square->getSquareIndex();
        for (auto move : _moves) {
            if (move.from == squareIndex) {
                ret = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
            }
        }
    }

    return ret;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = (ChessSquare *) &src;
    ChessSquare* dstSquare = (ChessSquare *) &dst;

    if (srcSquare && dstSquare) {
        int srcIndex = srcSquare->getSquareIndex();
        int dstIndex = dstSquare->getSquareIndex();
        
        for (auto move : _moves) {
            if (move.to == dstIndex && move.from == srcIndex) {
                return true;
            }
        }
    }

    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> moves;
    moves.reserve(32);

    std::string state = stateString();

    int playerColor = getCurrentPlayer()->playerNumber();

    BitboardElement pawns = BitboardElement(0ULL);
    BitboardElement knights = BitboardElement(0ULL);
    BitboardElement bishops = BitboardElement(0ULL);
    BitboardElement rooks = BitboardElement(0ULL);
    BitboardElement queens = BitboardElement(0ULL);
    BitboardElement king = BitboardElement(0ULL);

    const char *pieces = (playerColor == WHITE) ? "0PNBRQK" : "0pnbrqk";

    for (int i = 0; i < 64; i++) {
        if (state[i] == pieces[1]) {
            pawns |= 1ULL << i;
        } else if (state[i] == pieces[2]) {
            knights |= 1ULL << i;
        } else if (state[i] == pieces[3]) {
            bishops |= 1ULL << i;
        } else if (state[i] == pieces[4]) {
            rooks |= 1ULL << i;
        } else if (state[i] == pieces[5]) {
            queens |= 1ULL << i;
        } else if (state[i] == pieces[6]) {
            king |= 1ULL << i;
        }
    }

    BitboardElement unoccupiedSquares = BitboardElement(0ULL);

    for (int i = 0; i < 64; i++) {
        if (state[i] == '0') {
            unoccupiedSquares |= 1ULL << i;
        }
    }

    BitboardElement occupancy = BitboardElement(pawns.getData() | knights.getData() | bishops.getData() | 
        rooks.getData() | queens.getData() | king.getData());
    BitboardElement emptySquares = BitboardElement(~occupancy.getData());
    BitboardElement enemySquares = BitboardElement(~(unoccupiedSquares.getData() | occupancy.getData()));
    
    generateKnightMoves(moves, knights, emptySquares);
    generatePawnMoves(moves, pawns, emptySquares, enemySquares, playerColor);
    generateKingMoves(moves, king, emptySquares);

    return moves;
}

BitboardElement Chess::generateKnightMoveBitboard(int square)
{
    uint64_t data = 0ULL;

    int rank = square / 8;
    int file = square % 8;

    // All possible L-shapes from the position
    static const std::pair<int, int> knightOffsets[] = {
        {2, 1}, {-2, 1}, {2, -1}, {-2, -1},
        {1, 2}, {-1, 2}, {1, -2}, {-1, -2}
    };

    for (auto [dr, df] : knightOffsets) {
        int r = rank + dr, f  = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            data |= 1ULL << (r * 8 + f);
        }
    }
    
    return BitboardElement(data);
}

BitboardElement Chess::generateKingMoveBitboard(int square)
{
    uint64_t data = 0ULL;

    int rank = square / 8;
    int file = square % 8;

    // All possible one step moves from the position
    static const std::pair<int, int> directions[] = {
        {1, 0}, {1, 1}, {0, 1}, {-1, 0}, {-1, -1}, {0, -1}, {-1, 1}, {1, -1}
    };

    for (auto [dr, df] : directions) {
        int r = rank + dr, f  = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            data |= 1ULL << (r * 8 + f);
        }
    }
    
    return BitboardElement(data);
}

// Generate actual move objects from a bitboard
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, BitboardElement emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_knightBitboards[fromSquare].getData() & (emptySquares.getData()));
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, BitboardElement emptySquares)
{
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_kingBitboards[fromSquare].getData() & (emptySquares.getData()));
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generatePawnMoves(std::vector<BitMove> &moves, BitboardElement pawnsBoard, BitboardElement emptySquares, BitboardElement enemySquares, int color)
{
    if (!pawnsBoard.getData()) return;

    // Define constant bitmasks
    constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); // A file mask
    constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); // H file mask
    constexpr uint64_t Rank3(0x0000000000FF0000ULL); // Rank 3 mask
    constexpr uint64_t Rank6(0x0000FF0000000000ULL); // Rank 6 mask

    // Find all single moves, move pawns up or down depending on the color
    BitboardElement singleMoves = (color == WHITE) ? (pawnsBoard.getData() << 8) & emptySquares.getData() : (pawnsBoard.getData() >> 8) & emptySquares.getData();

    // Find all double moves, move pawns an extra space forward if they are in the right rank
    BitboardElement doubleMoves = (color == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() : ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();

    // Captures
    BitboardElement capturesLeft = (color == WHITE) ? ((pawnsBoard.getData() & NotAFile) << 7) & enemySquares.getData() : ((pawnsBoard.getData() & NotAFile) >> 9) & enemySquares.getData();
    BitboardElement capturesRight = (color == WHITE) ? ((pawnsBoard.getData() & NotHFile) << 9) & enemySquares.getData() : ((pawnsBoard.getData() & NotHFile) >> 7) & enemySquares.getData();

    // Store shifts in ints so that we can add moves
    int singleShift = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;

    addPawnBitboardMovesToList(moves, singleMoves, singleShift);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove> &moves, const BitboardElement board, int shift)
{
    if (!board.getData()) return;

    board.forEachBit([&](int toSquare) {
        int fromSquare = toSquare - shift;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}
