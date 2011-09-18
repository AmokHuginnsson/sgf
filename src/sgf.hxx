/*
---           `sgf' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	sgf.hxx - this file is integral part of `sgf' project.

	i.  You may not make any changes in Copyright information.
	ii. You must attach Copyright information to any part of every copy
	    of this software.

Copyright:

 You are free to use this program as is, you can redistribute binary
 package freely but:
  1. You cannot use any part of sources of this software.
  2. You cannot redistribute any part of sources of this software.
  3. No reverse engineering is allowed.
  4. If you want redistribute binary package you cannot demand any fees
     for this software.
     You cannot even demand cost of the carrier (CD for example).
  5. You cannot include it to any commercial enterprise (for example 
     as a free add-on to payed software or payed newspaper).
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.
*/

#ifndef SGF_HXX_INCLUDED
#define SGF_HXX_INCLUDED 1

#include <yaal/hcore/hstring.hxx>
#include <yaal/hcore/harray.hxx>
#include <yaal/hcore/htree.hxx>
#include <yaal/hcore/hstreaminterface.hxx>

namespace sgf {

char const* non_space( char const*, char const* );

class SGF {
public:
	struct GAME_TYPE {
		typedef enum {
			GO = 1,
			GOMOKU = 4
		} game_type_t;
	};
	struct ERROR {
		typedef enum {
			UNEXPECTED_EOF = 0,
			UNEXPECTED_DATA = 1,
			GT_OPEN_EXPECTED = 2,
			GT_CLOSE_EXPECTED = 3,
			PROP_IDENT_EXPECTED = 4,
			PROP_VAL_OPEN_EXPECTED = 5,
			PROP_VAL_CLOSE_EXPECTED = 6,
			NODE_MARK_EXPECTED = 7,
			BAD_GAME_TYPE = 8,
			BAD_FILE_FORMAT = 9
		} code_t;
	};
	struct TERM {
		static char const GT_OPEN = '(';
		static char const GT_CLOSE = ')';
		static char const PROP_VAL_OPEN = '[';
		static char const PROP_VAL_CLOSE = ']';
		static char const NODE_MARK = ';';
		static char const ESCAPE = '\\';
	};
	struct Game {
		struct Move {
			struct Player {
				typedef enum {
					BLACK,
					WHITE,
					UNSET
				} player_t;
			};
			char _coord[3];
			yaal::hcore::HString _comment;
			Move( void )
				: _coord(), _comment()
				{ }
			Move( int col_, int row_ )
				: _coord(), _comment() {
				_coord[0] = static_cast<char>( col_ + 'a' );
				_coord[1] = static_cast<char>( row_ + 'a' );
			}
			Move( char const* const coord_ )
				: _coord(), _comment()
				{ coord( coord_ ); }
			Move( yaal::hcore::HString const& coord_ )
				: _coord(), _comment()
				{ coord( coord_ ); }
			char const* coord( void ) const
				{ return ( _coord ); }
			void coord( yaal::hcore::HString const& coord_ ) {
				if ( coord_.get_length() >= 2 ) {
					_coord[0] = coord_[0];
					_coord[1] = coord_[1];
				} else
					_coord[0] = _coord[1] = 0;
			}
			void coord( char const* coord_ ) {
				_coord[0] = coord_[0];
				_coord[1] = coord_[1];
			}
			int col( void ) const {
				return ( _coord[0] - 'a' );
			}
			int row( void ) const {
				return ( _coord[1] - 'a' );
			}
		};
		typedef Move::Player Player;
		static int const RESIGN = 0xffff;
		static int const TIME = 0x7fff;
		typedef yaal::hcore::HArray<Move> preset_t;
		typedef yaal::hcore::HTree<Move> game_tree_t;
		yaal::hcore::HString _blackName;
		yaal::hcore::HString _whiteName;
		yaal::hcore::HString _blackRank;
		yaal::hcore::HString _whiteRank;
		preset_t _blackPreset;
		preset_t _whitePreset;
		game_tree_t _tree;
		Player::player_t _firstToMove;
		int _gobanSize;
		int _time;
		int _handicap;
		double _komi;
		int _result;
		yaal::hcore::HString _place;
		yaal::hcore::HString _comment;
		Game( void )
			: _blackName(), _whiteName(), _blackRank( "30k" ), _whiteRank( "30k" ),
			_blackPreset(), _whitePreset(), _tree(), _firstToMove( Player::UNSET ),
			_gobanSize( 19 ), _time( 0 ), _handicap( 0 ), _komi( 5.5 ),
			_result( 0 ), _place(), _comment()
			{ }
		void add_stone( Player::player_t, int, int );
		void add_stone( Player::player_t, Move const& );
		game_tree_t::node_t move( game_tree_t::node_t, Player::player_t, int, int );
		void clear( void );
	};
	typedef Game::Move Move;
	typedef Game::Move::Player Player;
private:
	GAME_TYPE::game_type_t _gameType;
	yaal::hcore::HString _rawData;
	char const* _beg;
	char const* _cur;
	char const* _end;
	yaal::hcore::HString _cache;
	yaal::hcore::HString _cachePropIdent;
	typedef yaal::hcore::HArray<yaal::hcore::HString> prop_values_t;
	prop_values_t _cachePropValue;
	Game _game;
	Game::game_tree_t::node_t _currentMove;
	yaal::hcore::HString _app;
public:
	SGF( GAME_TYPE::game_type_t, yaal::hcore::HString const& = "libsgf" );
	void load( yaal::hcore::HStreamInterface& );
	void save( yaal::hcore::HStreamInterface& );
	void clear( void );
	void add_stone( Player::player_t, int, int );
	Game::game_tree_t::node_t move( Game::game_tree_t::node_t, Player::player_t, int, int );
private:
	void parse_game_tree( void );
	void parse_sequence( void );
	void parse_node( void );
	void parse_property( void );
	yaal::hcore::HString const& parse_property_ident( void );
	void parse_property_value( prop_values_t& );
	void not_eof( void );
	void save_variations( Player::player_t, Game::game_tree_t::const_node_t, yaal::hcore::HStreamInterface& );
	void save_move( Player::player_t, Game::game_tree_t::const_node_t, yaal::hcore::HStreamInterface& );
	SGF( SGF const& );
	SGF& operator = ( SGF const& );
};

typedef yaal::hcore::HExceptionT<SGF> SGFException;

}

#endif /* #ifndef SGF_HXX_INCLUDED */

