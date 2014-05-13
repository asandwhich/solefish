// An attempt to implement the sunfish engine in c++11

#include <cstring>
#include <ctype.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

#define TABLE_SIZE 1000000
#define NODES_SEARCHED 10000
#define MATE_VALUE 30000

using namespace std;

// Definition of corners
const int A1 = 91, H1 = 98, A8 = 21, H8 = 28;

char initial[120] =  "                     rnbqkbnr  pppppppp  ........  ........  ........  ........  PPPPPPPP  RNBQKBNR                    ";


// Move and eval tables
const int N = -10;
const int E = 1;
const int S = 10;
const int W = -1;

// directions of movement for different pieces. 0s for pieces that don't have 8 directions
//int directions[6][8] = {  		// Shifting to map, TODO remove

map<char, vector<int> > directions;

// Entry definition
typedef struct Entry
{
	int depth = -1, score = -1, gamma = -1;
	pair<int, int> bmove = make_pair(-1,-1);
    Entry() : depth( -1 ), score( -1 ), gamma( -1 ), bmove( make_pair( -1, -1 ) ) {}
	Entry( int sDepth, int sScore, int sGamma, pair<int, int> sBmove ) :
		depth( sDepth ),
		score( sScore ),
		gamma( sGamma ),
		bmove( sBmove )
	{
	}
} Entry;

// Piece square tables, used for specifying values of pieces at certain squares
// Can be thought of as magic numbers for now
map<char, vector<int> > pst;


class Position
{

public:
    Position( char * sboard, int score, bool * wc, bool * bc, int ep, int kp );
    ~Position();

    vector<pair<int,int> > genMoves();  // generate moves only for noncpu pieces. White
    Position rotate();    // Flips white and black. White on top, black on bottom
    Position move( pair<int, int> sMove );
    int value( pair<int, int> sMove );
    // define < operator to please std::map
    bool operator <( const Position& rhs ) const
    {
        return mScore < rhs.mScore;
    }


// Public Variables
public:
    //   board -- a 120 char representation of the board
    // score -- the board evaluation
    // wc -- the castling rights
    // bc -- the opponent castling rights
    // ep - the en passant square
    // kp - the king passant square
    char mBoard[120];  // board
    int mScore;        // integer representing score?
    bool mWc[2];       // White castling rights, one for left, one for right?
    bool mBc[2];       // black castling rights. One for left, one for right
    int mEp;           // integer representing en passant square. 21-98
    int mKp;           // integer representing king passant square. 21-98



};

map<Position, Entry> tp; // Transposition table. Maps depth, best score, and gamma score to a certain board position

vector<pair<int, int> > Position::genMoves()
{
	vector<pair<int, int> > moves;
	// Iterate through mBoard.
	for( int i = 0; i < 120; i++ )
	{
		if( isupper( mBoard[i] ) )
		{
			vector<int> dirs = directions[mBoard[i]];
			for( int d = 0; d < dirs.size(); d++ )
			{
				int di = dirs[d];
				for( int j = (di+i); ; j += di )
				{
					char q = mBoard[j];
					
					// stay inside playable board
					if( q == ' ' )
					{
						break;
					}
					// Check castling
					if( i == A1 && q == 'K' && mWc[0] )
					{
						// yield (j, j-2)
						moves.push_back( make_pair( j, j-2 ) );
					}
					if( i == H1 && q == 'K' && mWc[1] )
					{
						// yield (j, j+2)
						moves.push_back( make_pair( j, j+2 ) );
					}
					// No friendly captures
					if( isupper( q ) )
					{
						break;
					}
					// Special pawn movement
					if( mBoard[i] == 'P' && ( di == N+W || di == N+E ) && q == '.' && ( j != mEp && j != mKp ) )
					{
						break;
					}
					if( mBoard[i] == 'P' && ( di == N || di == 2*N ) && q != '.' )
					{
						break;
					}
					if( mBoard[i] == 'P' && di == 2*N && ( i < A1+N || mBoard[i+N] != '.' ) )
					{
						break;
					}
					
					// Move the piece
					// yield (i, j)
					moves.push_back( make_pair( i, j ) );
					
					// Stop crawlers from sliding
					if( mBoard[i] == 'P' || mBoard[i] == 'N' || mBoard[i] == 'K' )
					{
						break;
					}
					
					// No sliding after captures
					if( islower( q ) )
					{
						break;
					}
				}
			}
		}
	}
	return moves; // placeholder
}

Position::Position( char * board, int score, bool * wc, bool * bc, int ep, int kp )
{
    strcpy_s( mBoard, board ); // strcpy_s to satisfy vs
    mScore = score;
    memcpy( mWc, wc, sizeof( mWc ) );
    memcpy( mBc, bc, sizeof( mBc ) );
    mEp = ep;
    mKp = kp;
}
Position::~Position()
{

}

// Return a new position with a rotated board
Position Position::rotate()
{
	// Reverse order of board array then swap the case of each character
	// then return Position( swappedboard, -mScore, mBc, mWc, 119 - mEp, 119 - mKp );
	//												^-----^---These two need to be swapped like they are here
	char swapBoard[120];
	// Reverse alg taken from http://stackoverflow.com/questions/1128985/c-reverse-array
	strcpy_s( swapBoard, mBoard );
	int len = strlen( swapBoard );
	for( int i = 0; i < len / 2; i++ )
	{
		swapBoard[i] ^= swapBoard[len - i - 1];
		swapBoard[len - i - 1] ^= swapBoard[i];
		swapBoard[i] ^= swapBoard[len - i - 1];
	}
    // Now need swap case of character.
    for( int i = 0; i < len; i++ )
    {
        if( isalpha( swapBoard[i] ) )
        {
            if( islower( swapBoard[i] ) )
            {
                swapBoard[i] = toupper( swapBoard[i] );
            }
            else if( isupper( swapBoard[i] ) )
            {
                swapBoard[i] = tolower( swapBoard[i] );
            }
        }
    }
	return Position( swapBoard, -mScore, mBc, mWc, 119 - mEp, 119 - mKp );
}

// Return new position with moved pieces
Position Position::move( pair<int, int> sMove )
{
	int i = sMove.first;
	int j = sMove.second;
	char p = mBoard[i];
	char q = mBoard[j];
	// put = lambda board, i, p: board[:i] + p + board[i+1:]
	// creates put functions that places a letter p at a certain point in the board i.
	// This doesn't need to be done in c++, as we are using a char array, instead of pythons immutable string.
	
	// Copy variables and reset ep and kp
	char board[120];
	strcpy_s( board, mBoard );
	int ep = 0, kp = 0;
	bool wc[2], bc[2];
	memcpy( wc, mWc, sizeof( wc ) );
    memcpy( bc, mBc, sizeof( bc ) );
	int score = mScore + value( sMove );
	// Actual move
	board[j] = board[i];
	board[i] = '.';
	// Castling rights
	if( i == A1 )
	{
		wc[0] = false;
	}
	if( i == H1 )
	{
		wc[1] = false;
	}
	if( j == A8 )
	{
		bc[1] = false;
	}
	if( j == H8 )
	{
		bc[0] = false;
	}
	if( p == 'K' )
	{
		wc[0] = false; wc[1] = false;
		if( abs( j - i ) == 2 )
		{
			kp = floor( (i+j) / 2 );
			board[ (j<i) ? A1 : H1 ] = '.';
			board[kp] = 'R';
		}
	}
    if( p == 'P' )
    {
        if( A8 <= j && j <= H8 )
        {
            board[j] = 'Q';
        }
        if( j - i == 2 * N )
        {
            ep = i + N;
        }
        if( ( j - i == N + W || j - i == N + E ) && q == '.' )
        {
            board[j + S] = '.';
        }
    }

    return Position( board, score, wc, bc, ep, kp ).rotate();
}

int Position::value( pair<int, int> sMove )
{
    int i = sMove.first;
    int j = sMove.second;
    char p = mBoard[i];
    char q = mBoard[j];

    // Actual move
    int score = pst[toupper(p)][j] - pst[toupper(p)][i];

    // Capture
    if( islower( q ) )
    {
        score += pst[toupper( q )][j];
    }
    if( abs( j - mKp ) < 2 )
    {
        score += pst['K'][j];
    }
    //Castling
    if( p == 'K' && abs( i - j ) == 2 )
    {
        score += pst['R'][floor( ( i + j ) / 2 )];
        score -= pst['R'][(j<i) ? A1 : H1];
    }
    // More special pawn movement
    if( p == 'P' )
    {
        if( A8 <= j && j <= H8 )
        {
            score += pst['Q'][j] - pst['P'][j];
        }
        if( j == mEp )
        {
            score += pst['P'][j + S];
        }
    }
    return score;
}

int nodes = 0; // Number of nodes that have been searched

int bound( Position pos, int gamma, int depth )
{
	nodes += 1;
	
	Entry initEntry = tp[pos];
	if( initEntry.depth != -1 && initEntry.depth >= depth && ( initEntry.score < initEntry.gamma && initEntry.score < gamma || initEntry.score >= initEntry.gamma && initEntry.score >= gamma ) )
	{
		return initEntry.score;
	}
	
	if( abs( pos.mScore ) >= MATE_VALUE )
	{
		return pos.mScore;
	}
	
	int nullscore = ( depth > 0 ) ? -bound(pos.rotate(), 1-gamma, depth-3 ) : pos.mScore;
	
	if( nullscore >= gamma )
	{
		return nullscore;
	}
	
	int best = -3 * MATE_VALUE;
	pair<int, int> bmove;
	// not able to do the sorting by value thing here yet. TODO
	vector<pair<int, int> > moves = pos.genMoves();
	for( int i = 0; i < moves.size(); i++ )
	{
		if( depth <= 0 && pos.value( moves[i] ) < 150 )
		{
			break;
		}
		int score = -bound( pos.move( moves[i] ), 1-gamma, depth-1 );
		if( score > best )
		{
			best = score;
			bmove = moves[i];
		}
		if( score >= gamma )
		{
			break;
		}
	}
	
	if( depth <= 0 && best < nullscore )
	{
		return nullscore;
	}
	// Look at this again. No idea what is happening here with the is None.
	if( depth > 0 && best <= -MATE_VALUE && nullscore > -MATE_VALUE )
	{
		best = 0;
	}
	
	if( initEntry.depth == -1 || depth >= initEntry.depth && best >= gamma )
	{
		tp[pos] = Entry( depth, best, gamma, bmove );
		if( tp.size() > TABLE_SIZE )
		{
			tp.erase( tp.end() ); // TODO change since not sorted
		}
	}
    return best; // returns integer score
}

// should return a move and a score
pair<pair<int, int>, int> search( Position pos, int maxn = NODES_SEARCHED )
{
	nodes = 0;
    int score;
	// Limit depth of search to a constant 99 so stack overflow isn't achieved.
	for( int depth = 1; depth < 99; depth++ )
	{
		// Inner loop is a binary search on the score of the position
		// Inv: lower <= score <= upper
		// This can be broken by values from the transposition table since they don't have the same concept of p(score).
		// As a result lower < upper - margin is used as the loop condition.
		int lower = -3 * MATE_VALUE;
		int upper = 3 * MATE_VALUE;
		while( lower < ( upper - 3 ) )
		{
			int gamma = floor( ( lower + upper + 1 ) / 2 );
			score = bound( pos, gamma, depth );
			if( score >= gamma )
			{
				lower = score;
			}
			if( score < gamma )
			{
				upper = score;
			}
		}
		
		// Stop increasing depth of search if global counter indicates too much time spent or if game is won
		if( nodes >= maxn || abs( score ) >= MATE_VALUE )
		{
			break;
		}
	}

    // If game hasn't finished retrieve move from transposition table
	Entry entry = tp[pos];
	if( entry.depth != -1 )
	{
		return make_pair( entry.bmove, score );
	}
	return make_pair( make_pair( 0, 0 ), score );
}

// Parses user input string sMove into two integers representing start position and end position
// Input should be of the form 'e2e4'. parse( 'e2e4' ) should return < 85, 65 >
pair<int, int> parse( string sMove )
{
	// Catch bad input, return move that can't exist
	if( sMove.length() != 4 )
	{
		return make_pair( 0, 0 );
	}
	// calculate starting position
	int file1 = sMove.at( 0 ) - 'a';
	int rank1 = ( sMove.at( 1 ) - '0' ) - 1;
	int pos1 = A1 + file1 - 10 * rank1;

	// calculate destination
	int file2 = sMove.at( 2 ) - 'a';
	int rank2 = ( sMove.at( 3 ) - '0' ) - 1;
	int pos2 = A1 + file2 - 10 * rank2;

	return make_pair( pos1, pos2 );
}

// Returns string of position
string render( int i )
{
	int rank = ( i - A1 ) / 10;
	int file = ( i - A1 ) % 10;
    char fil = file + 'a';
    return to_string(fil) + to_string( -rank + 1 );
}

void printBoard( char * board )
{
    // Assume board always has a length of 120
    for( int i = 0; i < 120; i++ )
    {
        printf( "%c ", board[i] );
        if( i % 10 == 9 )
        {
            printf( "\n" );
        }
    }
    printf( "\n" );
}

int main()
{
    // Define directions
    directions['P'] = { N, 2 * N, N + W, N + E };
    directions['B'] = { N + E, S + E, S + W, N + W };
    directions['N'] = { 2 * N + E, N + 2 * E, S + 2 * E, 2 * S + E, 2 * S + W, S + 2 * W, N + 2 * W, 2 * N + W };
    directions['R'] = { N, E, S, W };
    directions['Q'] = { N, E, S, W, N + E, S + E, S + W, N + W };
    directions['K'] = { N, E, S, W, N + E, S + E, S + W, N + W };

    // Define pst
    pst['P'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 198, 198, 198, 198, 198, 198, 198, 198, 0, 0, 178, 198, 198, 198, 198, 198, 198, 178, 0, 0, 178, 198, 198, 198, 198, 198, 198, 178, 0, 0, 178, 198, 208, 218, 218, 208, 198, 178, 0, 0, 178, 198, 218, 238, 238, 218, 198, 178, 0, 0, 178, 198, 208, 218, 218, 208, 198, 178, 0, 0, 178, 198, 198, 198, 198, 198, 198, 178, 0, 0, 198, 198, 198, 198, 198, 198, 198, 198, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    pst['B'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 797, 824, 817, 808, 808, 817, 824, 797, 0, 0, 814, 841, 834, 825, 825, 834, 841, 814, 0, 0, 818, 845, 838, 829, 829, 838, 845, 818, 0, 0, 824, 851, 844, 835, 835, 844, 851, 824, 0, 0, 827, 854, 847, 838, 838, 847, 854, 827, 0, 0, 826, 853, 846, 837, 837, 846, 853, 826, 0, 0, 817, 844, 837, 828, 828, 837, 844, 817, 0, 0, 792, 819, 812, 803, 803, 812, 819, 792, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    pst['N'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 627, 762, 786, 798, 798, 786, 762, 627, 0, 0, 763, 798, 822, 834, 834, 822, 798, 763, 0, 0, 817, 852, 876, 888, 888, 876, 852, 817, 0, 0, 797, 832, 856, 868, 868, 856, 832, 797, 0, 0, 799, 834, 858, 870, 870, 858, 834, 799, 0, 0, 758, 793, 817, 829, 829, 817, 793, 758, 0, 0, 739, 774, 798, 810, 810, 798, 774, 739, 0, 0, 683, 718, 742, 754, 754, 742, 718, 683, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    pst['R'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 1258, 1263, 1268, 1272, 1272, 1268, 1263, 1258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    pst['Q'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 2529, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    pst['K'] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60098, 60132, 60073, 60025, 60025, 60073, 60132, 60098, 0, 0, 60119, 60153, 60094, 60046, 60046, 60094, 60153, 60119, 0, 0, 60146, 60180, 60121, 60073, 60073, 60121, 60180, 60146, 0, 0, 60173, 60207, 60148, 60100, 60100, 60148, 60207, 60173, 0, 0, 60196, 60230, 60171, 60123, 60123, 60171, 60230, 60196, 0, 0, 60224, 60258, 60199, 60151, 60151, 60199, 60258, 60224, 0, 0, 60287, 60321, 60262, 60214, 60214, 60262, 60321, 60287, 0, 0, 60298, 60332, 60273, 60225, 60225, 60273, 60332, 60298, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


    bool initBools[] = { true, true };
    Position pos( initial, 0, initBools, initBools, 0, 0 );
    while( 1 )
    {
        printBoard( pos.mBoard );
        // Generate possible moves
		pair<int, int> move;
		vector<pair<int, int> > generatedMoves = pos.genMoves();
		while( find( generatedMoves.begin(), generatedMoves.end(), move ) == generatedMoves.end() )
		{
			// Get move from user
			string input;
            cout << "Input move: ";
			cin >> input;
			move = parse( input );
		}
		pos = pos.move( move );
		// After user move, print rotated board
		printBoard( pos.rotate().mBoard );

        pair<pair<int, int>, int> moveScore = search( pos );
        move = moveScore.first;
        int score = moveScore.second;
        if( score <= -MATE_VALUE )
        {
            printf( "You won" );
            return 0;
        }
        if( score >= MATE_VALUE )
        {
            printf( "You lost" );
            return 0;
        }
		
        //printf( "CPU Move: %s%s\n", render( 119 - move.first ), render( 119 - move.second ) );
        pos = pos.move( move );

    }

    return 0;
}