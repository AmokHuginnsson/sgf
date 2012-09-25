/*
---           `sgf' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	sgf.hxx - this file is integral part of `sgf' project.

  i.  You may not make any changes in Copyright information.
  ii. You must attach Copyright information to any part of every copy
      of this software.

Copyright:

 You can use this software free of charge and you can redistribute its binary
 package freely but:
  1. You are not allowed to use any part of sources of this software.
  2. You are not allowed to redistribute any part of sources of this software.
  3. You are not allowed to reverse engineer this software.
  4. If you want to distribute a binary package of this software you cannot
     demand any fees for it. You cannot even demand
     a return of cost of the media or distribution (CD for example).
  5. You cannot involve this software in any commercial activity (for example
     as a free add-on to paid software or newspaper).
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.
*/

#ifndef SGF_HXX_INCLUDED
#define SGF_HXX_INCLUDED 1

#include <yaal/hcore/hstring.hxx>
#include <yaal/hcore/harray.hxx>
#include <yaal/hcore/hmap.hxx>
#include <yaal/hcore/hstaticarray.hxx>
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
	struct Player {
		typedef enum {
			BLACK,
			WHITE,
			UNSET
		} player_t;
	};
	struct Position {
		typedef enum {
			REMOVE = 0,
			BLACK = 1,
			WHITE = 2,
			TRIANGLE = 3,
			SQUARE = 4,
			CIRCLE = 5
		} position_t;
	};
	struct Coord {
		char _data[3];
		Coord( void )
			: _data()
			{ }
		Coord( int col_, int row_ )
			: _data() {
			_data[0] = static_cast<char>( col_ + 'a' );
			_data[1] = static_cast<char>( row_ + 'a' );
		}
		Coord( char const* coord_ )
			: _data() {
			_data[0] = coord_[0];
			_data[1] = coord_[1];
		}
		Coord( yaal::hcore::HString const& coord_ )
			: _data() {
			if ( coord_.get_length() >= 2 ) {
				_data[0] = coord_[0];
				_data[1] = coord_[1];
			}
		}
		int col( void ) const {
			return ( _data[0] - 'a' );
		}
		int row( void ) const {
			return ( _data[1] - 'a' );
		}
		char const* data( void ) const {
			return ( _data );
		}
		void swap( Coord& c_ ) {
			if ( &c_ != this ) {
				using yaal::swap;
				swap( *reinterpret_cast<yaal::i16_t*>( _data ), *reinterpret_cast<yaal::i16_t*>( c_._data ) );
			}
		}
	};
	struct Setup {
		typedef yaal::hcore::HList<Coord> coords_t;
		typedef yaal::hcore::HMap<Position::position_t, coords_t> setup_t;
		setup_t _data;
		Setup( void )
			: _data()
			{}
		void swap( Setup& s_ ) {
			if ( &s_ != this ) {
				using yaal::swap;
				swap( _data, s_._data );
			}
		}
	};
	struct Move {
		Coord _coord;
		yaal::hcore::HString _comment;
		Setup* _setup;
		Move( void )
			: _coord(), _comment(), _setup( NULL )
			{}
		Move( Setup* setup_ )
			: _coord(), _comment(), _setup( setup_ )
			{}
		Move( Coord const& coord_ )
			: _coord( coord_ ), _comment(), _setup( NULL ) {
		}
		Move( char const* const coord_ )
			: _coord( coord_ ), _comment(), _setup( NULL )
			{ }
		Move( yaal::hcore::HString const& coord_ )
			: _coord( coord_ ), _comment(), _setup( NULL )
			{ }
		Move( Move const& m_ )
			: _coord( m_._coord ), _comment( m_._comment ), _setup( m_._setup )
			{ }
		Move& operator = ( Move const& m_ ) {
			if ( &m_ != this ) {
				Move tmp( m_ );
				swap( tmp );
			}
			return ( *this );
		}
		void swap( Move& );
		char const* coord( void ) const
			{ return ( _coord.data() ); }
		void set_coord( Coord const& coord_ ) {
			_coord = coord_;
		}
		int col( void ) const {
			return ( _coord.col() );
		}
		int row( void ) const {
			return ( _coord.row() );
		}
	};
	static int const RESIGN = 0xffff;
	static int const TIME = 0x7fff;
	typedef yaal::hcore::HArray<Move> preset_t;
	typedef yaal::hcore::HArray<Setup> setups_t;
	typedef yaal::hcore::HTree<Move> game_tree_t;
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
	game_tree_t::node_t _currentMove;
	yaal::hcore::HString _app;
	yaal::hcore::HString _blackName;
	yaal::hcore::HString _whiteName;
	yaal::hcore::HString _blackRank;
	yaal::hcore::HString _whiteRank;
	setups_t _setups;
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
public:
	SGF( GAME_TYPE::game_type_t, yaal::hcore::HString const& = "libsgf" );
	void swap( SGF& );
	void load( yaal::hcore::HStreamInterface& );
	void load( yaal::hcore::HString const& );
	void save( yaal::hcore::HStreamInterface&, bool = false );
	void move( Coord const& );
	void clear( void );
	void add_position( Position::position_t, Coord const& );
	void set_player( Player::player_t, yaal::hcore::HString const&, yaal::hcore::HString const& = "30k" );
	void set_info( Player::player_t, int = 19, int = 0, double = 5.5, int = 0, yaal::hcore::HString const& = yaal::hcore::HString() );
	void add_comment( yaal::hcore::HString const& );
private:
	void parse( void );
	game_tree_t::node_t move( game_tree_t::node_t, Coord const& );
	void parse_game_tree( void );
	void parse_sequence( void );
	void parse_node( void );
	void parse_property( void );
	yaal::hcore::HString const& parse_property_ident( void );
	void parse_property_value( prop_values_t& );
	void not_eof( void );
	void save_variations( Player::player_t, game_tree_t::const_node_t, yaal::hcore::HStreamInterface&, bool );
	void save_move( Player::player_t, game_tree_t::const_node_t, yaal::hcore::HStreamInterface&, bool );
	SGF( SGF const& );
	SGF& operator = ( SGF const& );
};

typedef yaal::hcore::HExceptionT<SGF> SGFException;

inline void swap( SGF& a, SGF& b )
	{ a.swap( b ); }

inline void swap( SGF::Move& a, SGF::Move& b )
	{ a.swap( b ); }

inline void swap( SGF::Setup& a, SGF::Setup& b )
	{ a.swap( b ); }

inline void swap( SGF::Coord& a, SGF::Coord& b )
	{ a.swap( b ); }

void banner( void );

}

#endif /* #ifndef SGF_HXX_INCLUDED */

