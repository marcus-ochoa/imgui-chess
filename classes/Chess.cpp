#include "Chess.h"
#include <limits>
#include <cmath>
#include <map>
#include <regex>

Chess::Chess()
{
    _grid = new Grid(8, 8);
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
        std::cout << match.size() << std::endl;
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
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
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
