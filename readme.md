# IMGUI Chess
Chess class project for CMPM123 course, based on [this](https://github.com/gdevine-ucsc/chess-base)

## AI Update
Negamax AI with alpha-beta pruning and a simple combination piece square and material score evaluator is working. On my laptop I am able to run the AI to a depth of 5 comfortably with longer evaluations taking several seconds, however a depth of 6 takes minutes for longer evaluations. The AI evaluates around 13 million boards per second on average. As a chess novice (who knows little more than the rules of the game), the AI can soundly beat me most of the time. In my novice opinion the AI seems to take somewhat risky moves and does not have a good sense of general board and pawn structure. When I played the AI against Stockfish, Stockfish soundly beat it by exposing these weaknesses. Making the AI I generally followed what was done in class and used the given bitboard and magic bitboard classes. I added separate piece square boards for the white and black pieces to allow for evalution without branching as recommended in class. My main challenges were caused by hang ups with smaller things such as making sure I understood how the piece square board arrays were laid out compared to the state string and making sure that the generate moves function worked whether I passed in the player color as 0 or -1 for white since I setup white as 0 initially.
![Gif](/screenshots/ai-demo.gif)

## Move Update
Simple moves (Knight, Pawns, King) are working. Followed the approaches from class, used the bitboard approach for pawns.
![Screenshot](/screenshots/basic-moves-breakpoint.png "Screenshot")
![Screenshot](/screenshots/basic-moves-board.png "Screenshot")

## FEN String Update
FEN string to board is working. I used regex to get all the fields, although I am only using the first field with board layout for now.
![Screenshot](/screenshots/fen-string-setup.png "Screenshot")

## Details
Platform: Linux
