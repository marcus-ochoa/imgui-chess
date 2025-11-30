#include "Chess.h"
#include "PieceSquare.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>
#include <map>
#include <regex>
#include <chrono>
#include <iomanip>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();

    for (int i = 0; i < 128; i++) { _bitboardLookup[i] = 0; }

    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;

    _evaluateScores['P'] = 100;
    _evaluateScores['N'] = 300;
    _evaluateScores['B'] = 400;
    _evaluateScores['R'] = 500;
    _evaluateScores['Q'] = 900;
    _evaluateScores['K'] = 2000;
    _evaluateScores['p'] = -100;
    _evaluateScores['n'] = -300;
    _evaluateScores['b'] = -400;
    _evaluateScores['r'] = -500;
    _evaluateScores['q'] = -900;
    _evaluateScores['k'] = -2000;
    _evaluateScores['0'] = 0;

    _pieceSquareTables['P'] = pawnTableWhite;
    _pieceSquareTables['N'] = knightTableWhite;
    _pieceSquareTables['B'] = bishopTableWhite;
    _pieceSquareTables['R'] = rookTableWhite;
    _pieceSquareTables['Q'] = queenTableWhite;
    _pieceSquareTables['K'] = kingTableWhite;
    _pieceSquareTables['p'] = pawnTableBlack;
    _pieceSquareTables['n'] = knightTableBlack;
    _pieceSquareTables['b'] = bishopTableBlack;
    _pieceSquareTables['r'] = rookTableBlack;
    _pieceSquareTables['q'] = queenTableBlack;
    _pieceSquareTables['k'] = kingTableBlack;
    _pieceSquareTables['0'] = emptyTable;
}

Chess::~Chess()
{
    cleanupMagicBitboards();
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

    std::string state = stateString();
    _moves = generateAllMoves(state, getCurrentPlayer()->playerNumber());

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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

// Overriding this function to allow for regeneration of moves
void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    endTurn();
    std::string state = stateString();
    _moves = generateAllMoves(state, getCurrentPlayer()->playerNumber());
}

void Chess::clearBoardHighlights()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->setHighlighted(false);
    });
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

std::vector<BitMove> Chess::generateAllMoves(std::string& state, int playerColor)
{
    playerColor = (playerColor + 1) >> 1; // Converts -1 to 0 for when AI calls this function

    std::vector<BitMove> moves;
    moves.reserve(32);

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        _bitboards[_bitboardLookup[state[i]]] |= 1ULL << i;
    }

    _bitboards[WHITE_ALL_PIECES] = _bitboards[WHITE_PAWNS] |
        _bitboards[WHITE_KNIGHTS] |
        _bitboards[WHITE_BISHOPS] |
        _bitboards[WHITE_ROOKS] |
        _bitboards[WHITE_QUEENS] |
        _bitboards[WHITE_KING];

    _bitboards[BLACK_ALL_PIECES] = _bitboards[BLACK_PAWNS] |
        _bitboards[BLACK_KNIGHTS] |
        _bitboards[BLACK_BISHOPS] |
        _bitboards[BLACK_ROOKS] |
        _bitboards[BLACK_QUEENS] |
        _bitboards[BLACK_KING];

    _bitboards[OCCUPANCY] = _bitboards[WHITE_ALL_PIECES] | _bitboards[BLACK_ALL_PIECES];
    _bitboards[EMPTY_SQUARES] = ~_bitboards[OCCUPANCY];
    
    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + playerColor], ~_bitboards[WHITE_ALL_PIECES + playerColor]);
    generatePawnMoves(moves, _bitboards[WHITE_PAWNS + playerColor], _bitboards[EMPTY_SQUARES], _bitboards[BLACK_ALL_PIECES - playerColor], playerColor);
    generateKingMoves(moves, _bitboards[WHITE_KING + playerColor], ~_bitboards[WHITE_ALL_PIECES + playerColor]);
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + playerColor], _bitboards[OCCUPANCY], _bitboards[WHITE_ALL_PIECES + playerColor]);
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + playerColor], _bitboards[OCCUPANCY], _bitboards[WHITE_ALL_PIECES + playerColor]);
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + playerColor], _bitboards[OCCUPANCY], _bitboards[WHITE_ALL_PIECES + playerColor]);

    return moves;
}

// Generate actual move objects from a bitboard
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, BitboardElement emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(KnightAttacks[fromSquare] & (emptySquares.getData()));
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, BitboardElement emptySquares)
{
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(KingAttacks[fromSquare] & (emptySquares.getData()));
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies)
{
    piecesBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getBishopAttacks(fromSquare, occupancy.getData()) & ~friendlies.getData());
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies)
{
    piecesBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getRookAttacks(fromSquare, occupancy.getData()) & ~friendlies.getData());
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, BitboardElement occupancy, BitboardElement friendlies)
{
    piecesBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getQueenAttacks(fromSquare, occupancy.getData()) & ~friendlies.getData());
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}

void Chess::generatePawnMoves(std::vector<BitMove> &moves, BitboardElement pawnsBoard, BitboardElement emptySquares, BitboardElement enemySquares, int color)
{
    if (!pawnsBoard.getData()) return;

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

//
// this is the function that will be called by the AI
//
void Chess::updateAI()
{
    int bestVal = negInfinite;
    BitMove bestMove;
    std::string state = stateString();

    // Set time and move tracking variables
    const auto searchStart = std::chrono::steady_clock::now();
    _countMoves = 0;

    for (auto move : _moves) {

        // Save previous state
        char oldDestination = state[move.to];
        char pieceMoving = state[move.from];

        // Make the move
        state[move.to] = pieceMoving;
        state[move.from] = '0';


        int moveVal = -negamax(state, MAX_DEPTH, negInfinite, posInfinite, HUMAN_PLAYER);

        // Undo the move
        state[move.to] = oldDestination;
        state[move.from] = pieceMoving;
        
        // If the value of the current move is more than the best value, update best
        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    if (bestVal != negInfinite) {
        // Calculate and print out boards per second
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStart).count();
        const double boardsPerSecond = seconds > 0.0 ? static_cast<double>(_countMoves) / seconds : 0.0;
        std::cout << "Moves checked: " << _countMoves << " (" << std::fixed << std::setprecision(2) << boardsPerSecond << " boards/s)" << std::defaultfloat << std::endl;

        // Make best move
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}

int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor)
{
    _countMoves++;

    // Base case
    if (depth == 0) {
        // Multiply by negative player color because evaluate function evaluates for white
        return evaluateBoard(state) * -playerColor;
    }

    // Generate moves for this board state
    auto newMoves = generateAllMoves(state, playerColor);

    int bestVal = negInfinite; // Min value
    
    for (auto move : newMoves) {

        // Save previous state
        char oldDestination = state[move.to];
        char pieceMoving = state[move.from];

        // Make the move
        state[move.to] = pieceMoving;
        state[move.from] = '0';

        // Recursively evaluate (note the negation and flipped player color)
        bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, -playerColor));

        // Undo the move
        state[move.to] = oldDestination;
        state[move.from] = pieceMoving;

        // Alpha-beta pruning
        alpha = std::max(alpha, bestVal);

        if (alpha >= beta) {
            break;
        }
    }

    return bestVal;
}

int Chess::evaluateBoard(const std::string& state)
{
    int value = 0;
    int square = 0;
    for (char ch : state) {
        value += _evaluateScores[ch];
        value += _pieceSquareTables[ch][square];
        square++;
    }

    return value;
}
