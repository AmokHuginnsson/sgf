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
	typedef SGF this_type;
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
			BAD_FILE_FORMAT = 9,
			BAD_OVERTIME_DEFINITION = 10,
			MIXED_NODE = 11,
			DUPLICATED_COORDINATE = 12,
			MOVE_OUT_OF_RECORD = 13,
			MALFORMED_LABEL = 14,
			INCONSISTENT_FIRST_MOVE = 15
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
			WHITE
		} player_t;
	};
	struct Position {
		typedef enum {
			REMOVE = 0,
			BLACK = 1,
			WHITE = 2,
			TRIANGLE = 3,
			SQUARE = 4,
			CIRCLE = 5,
			MARK = 6,
			BLACK_TERITORY = 7,
			WHITE_TERITORY = 8
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
		bool operator == ( Coord const& c_ ) const {
			return ( ( _data[0] == c_._data[0] ) && ( _data[1] == c_._data[1] ) );
		}
		bool operator != ( Coord const& c_ ) const {
			return ( ( _data[0] != c_._data[0] ) || ( _data[1] != c_._data[1] ) );
		}
	};
	static Coord const PASS;
	struct Move;
	struct Setup {
		typedef yaal::hcore::HList<Coord> coords_t;
		typedef yaal::hcore::HMap<Position::position_t, coords_t> setup_t;
		typedef yaal::hcore::HPair<Coord, yaal::hcore::HString> label_t;
		typedef yaal::hcore::HList<label_t> labels_t;
		setup_t _data;
		labels_t _labels;
		Setup( void )
			: _data(), _labels()
			{}
		void swap( Setup& s_ ) {
			if ( &s_ != this ) {
				using yaal::swap;
				swap( _data, s_._data );
				swap( _labels, s_._labels );
			}
		}
	private:
		void add_position( Position::position_t, Coord const& );
		void add_label( label_t const& );
		friend struct Move;
	};
	class Move {
	public:
		struct TYPE {
			typedef enum {
				INVALID,
				MOVE,
				SETUP
			} type_t;
		};
	private:
		TYPE::type_t _type;
		Coord _coord;
		yaal::hcore::HString _comment;
		Setup* _setup;
		int _time;
	public:
		Move( void )
			: _type( TYPE::INVALID ), _coord(), _comment(), _setup( NULL ), _time( 0 )
			{}
		Move( Setup* setup_ )
			: _type( TYPE::SETUP ), _coord(), _comment(), _setup( setup_ ), _time( 0 )
			{}
		Move( Coord const& coord_ )
			: _type( TYPE::MOVE ), _coord( coord_ ), _comment(), _setup( NULL ), _time( 0 )
			{ }
		Move( char const* const coord_ )
			: _type( TYPE::MOVE ), _coord( coord_ ), _comment(), _setup( NULL ), _time( 0 )
			{ }
		Move( yaal::hcore::HString const& coord_ )
			: _type( TYPE::MOVE ), _coord( coord_ ), _comment(), _setup( NULL ), _time( 0 )
			{ }
		Move( Move const& m_ )
			: _type( m_._type ), _coord( m_._coord ), _comment( m_._comment ), _setup( m_._setup ), _time( m_._time )
			{ }
		Move& operator = ( Move const& m_ ) {
			if ( &m_ != this ) {
				Move tmp( m_ );
				swap( tmp );
			}
			return ( *this );
		}
		void swap( Move& );
		Coord const& coord( void ) const
			{ return ( _coord ); }
		void set_coord( Coord const& );
		void set_setup( Setup* );
		void add_comment( yaal::hcore::HString const& );
		void set_time( int );
		void add_position( Position::position_t, Coord const& );
		void add_label( Setup::label_t const& );
		void clear_markers( Position::position_t );
		int col( void ) const {
			return ( _coord.col() );
		}
		int row( void ) const {
			return ( _coord.row() );
		}
		yaal::hcore::HString const& comment( void ) const {
			return ( _comment );
		}
		TYPE::type_t type( void ) const {
			return ( _type );
		}
		Setup* setup( void ) {
			return ( _setup );
		}
		Setup const* setup( void ) const {
			return ( _setup );
		}
		int time( void ) const {
			return ( _time );
		}
	};
	static int const RESIGN = 0xffff;
	static int const TIME = 0x7fff;
	typedef yaal::hcore::HList<Setup> setups_t;
	typedef yaal::hcore::HTree<Move> game_tree_t;
	typedef yaal::hcore::HPair<int, int> byoyomi_t;
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
	yaal::hcore::HString _gameName;
	yaal::hcore::HString _date;
	yaal::hcore::HString _event;
	yaal::hcore::HString _round;
	yaal::hcore::HString _source;
	yaal::hcore::HString _author;
	yaal::hcore::HString _rules;
	yaal::hcore::HString _overTime;
	yaal::hcore::HString _blackName;
	yaal::hcore::HString _whiteName;
	yaal::hcore::HString _blackRank;
	yaal::hcore::HString _whiteRank;
	setups_t _setups;
	game_tree_t _tree;
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
	void move( Coord const&, int = 0 );
	void clear( void );
	void add_position( Position::position_t, Coord const& );
	void add_label( Setup::label_t const& );
	void set_player( Player::player_t, yaal::hcore::HString const&, yaal::hcore::HString const& = "30k" );
	void set_info( int = 19, int = 0, double = 5.5, int = 0, int = 0, int = 0, yaal::hcore::HString const& = yaal::hcore::HString() );
	void set_board_size( int );
	void set_komi( double );
	void set_handicap( int );
	void set_time( int );
	void set_overtime( int, int );
	void set_overtime( yaal::hcore::HString const& );
	yaal::hcore::HString const& get_overtime( void ) const;
	byoyomi_t get_byoyomi( void ) const;
	int get_board_size( void ) const;
	double get_komi( void ) const;
	int get_handicap( void ) const;
	int get_time( void ) const;
	void add_comment( yaal::hcore::HString const& );
	game_tree_t const& game_tree( void ) const;
	game_tree_t::const_node_t get_current_move( void ) const;
	void set_current_move( game_tree_t::const_node_t );
	void clear_markers( game_tree_t::const_node_t );
private:
	void clear_game( void );
	bool is_first_move( void ) const;
	Player::player_t first_to_move( void ) const;
	void parse( void );
	void parse_game_tree( void );
	void parse_sequence( void );
	void parse_node( void );
	void parse_property( void );
	yaal::hcore::HString const& parse_property_ident( void );
	void parse_property_value( prop_values_t& );
	void not_eof( void );
	void save_variations( Player::player_t, game_tree_t::const_node_t, yaal::hcore::HStreamInterface&, bool );
	void save_setup( game_tree_t::const_node_t, yaal::hcore::HStreamInterface&, bool );
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

