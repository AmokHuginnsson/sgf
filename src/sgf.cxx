/*
---           `sgf' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	sgf.cxx - this file is integral part of `sgf' project.

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

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <yaal/hcore/hfile.hxx>
#include <yaal/hcore/hcore.hxx>
#include <yaal/tools/tools.hxx>
M_VCSID( "$Id: "__ID__" $" )
#include "sgf.hxx"
#include "config.hxx"

using namespace yaal;
using namespace yaal::hcore;

namespace sgf {

char const _errMsg_[][50] = {
	"Unexpected end of file.",
	"Unexpected data bytes found.",
	"Expected GameTree opening sequence.",
	"Expected GameTree closing sequence.",
	"Expected property identifier.",
	"Expected property value opening sequence.",
	"Expected property value closing sequence.",
	"Expected node start marker sequence.",
	"Bad game type.",
	"Bad file format."
};

SGF::SGF( GAME_TYPE::game_type_t gameType_, HString const& app_ )
	: _gameType( gameType_ ), _rawData(), _beg( NULL ), _cur( NULL ), _end( NULL ),
	_cache(), _cachePropIdent(), _cachePropValue(), _game(),
	_currentMove( NULL ), _app( app_ ) {
}

void SGF::swap( SGF& sgf_ ) {
	M_PROLOG
	if ( &sgf_ != this ) {
		using yaal::swap;
		swap( _gameType, sgf_._gameType );
		swap( _rawData, sgf_._rawData );
		swap( _beg, sgf_._beg );
		swap( _cur, sgf_._cur );
		swap( _end, sgf_._end );
		swap( _cache, sgf_._cache );
		swap( _cachePropIdent, sgf_._cachePropIdent );
		swap( _cachePropValue, sgf_._cachePropValue );
		swap( _game, sgf_._game );
		swap( _currentMove, sgf_._currentMove );
		swap( _app, sgf_._app );
	}
	return;
	M_EPILOG
}

void SGF::move( int col_, int row_ ) {
	M_PROLOG
	if ( ! _currentMove )
		_currentMove = _game._tree.create_new_root();
	_currentMove = &*_currentMove->add_node( Move( col_, row_ ) );
	return;
	M_EPILOG
}

void SGF::set_player( Player::player_t player_, yaal::hcore::HString const& name_, yaal::hcore::HString const& rank_ ) {
	M_PROLOG
	if ( player_ == Player::BLACK ) {
		_game._blackName = name_;
		_game._blackRank = rank_;
	} else {
		_game._whiteName = name_;
		_game._whiteRank = rank_;
	}
	return;
	M_EPILOG
}

void SGF::set_info( Player::player_t player_, int gobanSize_, int handicap_, double komi_, int time_, yaal::hcore::HString const& place_ ) {
	M_PROLOG
	_game._firstToMove = player_;
	_game._gobanSize = gobanSize_;
	_game._handicap = handicap_;
	_game._komi = komi_;
	_game._time = time_;
	_game._place = place_;
	return;
	M_EPILOG
}

void SGF::add_comment( yaal::hcore::HString const& comment_ ) {
	M_PROLOG
	_game._comment += comment_;
	return;
	M_EPILOG
}

char const* non_space( char const* first, char const* last ) {
	for ( ; first != last ; ++ first ) {
		if ( ! ::memchr( _whiteSpace_.data(), *first, _whiteSpace_.size() ) )
			break;
	}
	return ( first );
}

void SGF::load( HStreamInterface& stream_ ) {
	M_PROLOG
	static int const BUFFER_SIZE( 4096 );
	HChunk buffer( BUFFER_SIZE );
	int nRead( 0 );
	clear();
	while ( ( nRead = static_cast<int>( stream_.read( buffer.raw(), BUFFER_SIZE ) ) ) > 0 )
		_rawData.append( buffer.raw(), nRead );
	parse();
	return;
	M_EPILOG
}

void SGF::load( HString const& data_ ) {
	M_PROLOG
	clear();
	_rawData = data_;
	parse();
	return;
	M_EPILOG
}

void SGF::parse( void ) {
	M_PROLOG
	_beg = _rawData.begin();
	_cur = _rawData.begin();
	_end = _rawData.end();
	clog << _rawData << endl;
	_cur = non_space( _cur, _end );
	try {
		parse_game_tree();
		while ( _cur != _end )
			parse_game_tree();
	} catch ( SGFException const& e ) {
		if ( _cur != _end )
			cerr << "Failed at byte: `" << *_cur << "'" << endl;
		throw;
	}
	return;
	M_EPILOG
}

void SGF::clear( void ) {
	M_PROLOG
	_beg = _cur = _end = NULL;
	_currentMove = NULL;
	_rawData.clear();
	_game.clear();
	return;
	M_EPILOG
}

void SGF::not_eof( void ) {
	if ( _cur == _end )
		throw SGFException( _errMsg_[ERROR::UNEXPECTED_EOF], static_cast<int>( _cur - _beg ) );
	return;
}

void SGF::parse_game_tree( void ) {
	M_PROLOG
	not_eof();
	if ( *_cur != TERM::GT_OPEN )
		throw SGFException( _errMsg_[ERROR::GT_OPEN_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	not_eof();
	if ( ! _currentMove )
		_currentMove = _game._tree.create_new_root();
	parse_sequence();
	Game::game_tree_t::node_t preVariationMove( _currentMove );
	while ( ( _cur != _end ) && ( *_cur != TERM::GT_CLOSE ) ) {
		_currentMove = &*preVariationMove->add_node();
		parse_game_tree();
		not_eof();
	}
	if ( *_cur != TERM::GT_CLOSE )
		throw SGFException( _errMsg_[ERROR::GT_CLOSE_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	return;
	M_EPILOG
}

void SGF::parse_sequence( void ) {
	M_PROLOG
	parse_node();
	_cur = non_space( _cur, _end );
	not_eof();
	while ( *_cur == TERM::NODE_MARK ) {
		_currentMove = &*_currentMove->add_node();
		parse_node();
		_cur = non_space( _cur, _end );
		not_eof();
	}
	return;
	M_EPILOG
}

void SGF::parse_node( void ) {
	M_PROLOG
	if ( *_cur != TERM::NODE_MARK )
		throw SGFException( _errMsg_[ERROR::NODE_MARK_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	not_eof();
	while ( ( *_cur != TERM::GT_CLOSE ) && ( *_cur != TERM::GT_OPEN ) && ( *_cur != TERM::NODE_MARK ) ) {
		parse_property();
		_cur = non_space( _cur, _end );
		not_eof();
	}
	return;
	M_EPILOG
}

void SGF::parse_property( void ) {
	M_PROLOG
	_cachePropIdent = parse_property_ident();
	if ( _cachePropIdent.is_empty() )
		throw SGFException( _errMsg_[ERROR::PROP_IDENT_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( _cur, _end );
	not_eof();
	_cachePropValue.clear();
	parse_property_value( _cachePropValue );
	while ( *_cur == TERM::PROP_VAL_OPEN )
		parse_property_value( _cachePropValue );
	HString const& singleValue( _cachePropValue[0] );
	if ( _cachePropIdent == "GM" ) {
		if ( lexical_cast<int>( singleValue ) != _gameType )
			throw SGFException( _errMsg_[ERROR::BAD_GAME_TYPE], static_cast<int>( _cur - _beg ) );
	} else if ( _cachePropIdent == "FF" ) {
		int ff( lexical_cast<int>( singleValue ) );
		if ( ( ff < 1 ) || ( ff > 4 ) )
			throw SGFException( _errMsg_[ERROR::BAD_FILE_FORMAT], static_cast<int>( _cur - _beg ) );
	} else if ( _cachePropIdent == "PB" )
		_game._blackName = singleValue;
	else if ( _cachePropIdent == "PW" )
		_game._whiteName = singleValue;
	else if ( _cachePropIdent == "BR" )
		_game._blackRank = singleValue;
	else if ( _cachePropIdent == "WR" )
		_game._whiteRank = singleValue;
	else if ( _cachePropIdent == "KM" )
		_game._komi = lexical_cast<double>( singleValue );
	else if ( _cachePropIdent == "HA" )
		_game._handicap = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "SZ" )
		_game._gobanSize = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "TM" )
		_game._time = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "PC" )
		_game._place = singleValue;
	else if ( _cachePropIdent == "RE" ) {
		if ( isdigit( singleValue[2] ) )
			_game._result = lexical_cast<int>( singleValue.raw() + 2 );
		else {
			char r( static_cast<char>( toupper( singleValue[2] ) ) );
			if ( r == 'R' )
				_game._result = Game::RESIGN;
			else if ( r == 'T' )
				_game._result = Game::TIME;
		}
		char player( static_cast<char>( toupper( singleValue[0] ) ) );
		if ( player == 'W' )
			_game._result = -_game._result;
	} else if ( _cachePropIdent == "AB" ) {
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it )
			_game.add_stone( Player::BLACK, Move( *it ) );
	} else if ( _cachePropIdent == "AW" ) {
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it )
			_game.add_stone( Player::WHITE, Move( *it ) );
	} else if ( _cachePropIdent == "B" ) {
		if ( _game._firstToMove == Player::UNSET )
			_game._firstToMove = Player::BLACK;
		(**_currentMove).coord( singleValue );
	} else if ( _cachePropIdent == "W" ) {
		if ( _game._firstToMove == Player::UNSET )
			_game._firstToMove = Player::WHITE;
		(**_currentMove).coord( singleValue );
	} else if ( _cachePropIdent == "C" ) {
		if ( _game._firstToMove != Player::UNSET )
			(**_currentMove)._comment = singleValue;
		else
			_game._comment += singleValue;
	} else
		clog << "property: `" << _cachePropIdent << "' = `" << singleValue << "'" << endl;
	return;
	M_EPILOG
}

HString const& SGF::parse_property_ident( void ) {
	M_PROLOG
	_cache.clear();
	while ( ( _cur != _end ) && isupper( *_cur ) ) {
		_cache += *_cur;
		++ _cur;
	}
	return ( _cache );
	M_EPILOG
}

void SGF::parse_property_value( prop_values_t& values_ ) {
	M_PROLOG
	not_eof();
	if ( *_cur != TERM::PROP_VAL_OPEN )
		throw SGFException( _errMsg_[ERROR::PROP_VAL_OPEN_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	not_eof();
	_cache.clear();
	bool escaped( false );
	while ( ( _cur != _end ) && ( escaped || ( *_cur != TERM::PROP_VAL_CLOSE ) ) ) {
		escaped = ( *_cur == TERM::ESCAPE );
		if ( ! escaped )
			_cache += *_cur;
		++ _cur;
	}
	not_eof();
	if ( *_cur != TERM::PROP_VAL_CLOSE )
		throw SGFException( _errMsg_[ERROR::PROP_VAL_CLOSE_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	values_.push_back( _cache );
	M_EPILOG
}

void SGF::save( HStreamInterface& stream_, bool noNL_ ) {
	M_PROLOG
	stream_ << "(;GM[" << static_cast<int>( _gameType ) << "]FF[4]AP[" << _app << ( noNL_ ? "]" : "]\n" )
		<< "SZ[" << _game._gobanSize << "]KM[" << setw( 1 ) << _game._komi << "]TM[" << _game._time << ( noNL_ ? "]" : "]\n" )
		<< "PB[" << _game._blackName << "]PW[" << _game._whiteName << ( noNL_ ? "]" : "]\n" )
		<< "BR[" << _game._blackRank << "]WR[" << _game._whiteRank << ( noNL_ ? "]" : "]\n" );
	if ( ! _game._comment.is_empty() ) {
		_cache = _game._comment;
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
	}
	if ( ! _game._blackPreset.empty() ) {
		stream_ << "AB";
		for ( Game::preset_t::const_iterator it( _game._blackPreset.begin() ), end( _game._blackPreset.end() ); it != end; ++ it )
			stream_ << '[' << it->coord() << ']';
	}
	if ( ! _game._whitePreset.empty() ) {
		stream_ << "AW";
		for ( Game::preset_t::const_iterator it( _game._whitePreset.begin() ), end( _game._whitePreset.end() ); it != end; ++ it )
			stream_ << '[' << it->coord() << ']';
	}
	if ( ! _game._tree.is_empty() )
		save_variations( _game._firstToMove, _game._tree.get_root(), stream_, noNL_ );
	stream_ << ( noNL_ ? ")" : ")\n" );
	return;
	M_EPILOG
}

void SGF::save_move( Player::player_t of_, Game::game_tree_t::const_node_t node_, HStreamInterface& stream_, bool ) {
	M_PROLOG
	stream_ << ';' << ( of_ == Player::BLACK ? 'B' : 'W' ) << '[' << (**node_).coord() << ']';
	if ( ! (**node_)._comment.is_empty() ) {
		_cache = (**node_)._comment;
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
	}
	return;
	M_EPILOG
}

void SGF::save_variations( Player::player_t from_, Game::game_tree_t::const_node_t node_, HStreamInterface& stream_, bool noNL_ ) {
	M_PROLOG
	int childCount( 0 );
	while ( ( childCount = static_cast<int>( node_->child_count() ) ) == 1 ) {
		node_ = &*node_->begin();
		save_move( from_, node_, stream_, noNL_ );
		from_ = ( from_ == Player::BLACK ? Player::WHITE : Player::BLACK );
	}
	if ( childCount > 1 ) /* We have variations. */ {
		for ( Game::game_tree_t::HNode::const_iterator it( node_->begin() ), end( node_->end() ); it != end; ++ it ) {
			stream_ << ( noNL_ ? "(" : "\n(" );
			save_move( from_, &*it, stream_, noNL_ );
			save_variations( ( from_ == Player::BLACK ? Player::WHITE : Player::BLACK ), &*it, stream_, noNL_ );
			stream_ << ( noNL_ ? ")" : ")\n" );
		}
	}
	return;
	M_EPILOG
}

void SGF::add_stone( Player::player_t player_, int col_, int row_ ) {
	M_PROLOG
	_game.add_stone( player_, col_, row_ );
	return;
	M_EPILOG
}

SGF::Game::game_tree_t::node_t SGF::move( Game::game_tree_t::node_t node_, int col_, int row_ ) {
	M_PROLOG
	return ( _game.move( node_, col_, row_ ) );
	M_EPILOG
}

SGF::Game::Game( void )
	: _blackName(), _whiteName(), _blackRank( "30k" ), _whiteRank( "30k" ),
	_blackPreset(), _whitePreset(), _tree(), _firstToMove( Player::UNSET ),
	_gobanSize( 19 ), _time( 0 ), _handicap( 0 ), _komi( 5.5 ),
	_result( 0 ), _place(), _comment()
	{ }

void SGF::Game::swap( Game& game_ ) {
	M_PROLOG
	if ( &game_ != this ) {
		using yaal::swap;
		swap( _blackName, game_._blackName );
		swap( _whiteName, game_._whiteName );
		swap( _blackRank, game_._blackRank );
		swap( _whiteRank, game_._whiteRank );
		swap( _blackPreset, game_._blackPreset );
		swap( _whitePreset, game_._whitePreset );
		swap( _tree, game_._tree );
		swap( _firstToMove, game_._firstToMove );
		swap( _gobanSize, game_._gobanSize );
		swap( _time, game_._time );
		swap( _handicap, game_._handicap );
		swap( _komi, game_._komi );
		swap( _result, game_._result );
		swap( _place, game_._place );
		swap( _comment, game_._comment );
	}
	return;
	M_EPILOG
}

void SGF::Game::Move::swap( Move& move_ ) {
	M_PROLOG
	if ( &move_ != this ) {
		using yaal::swap;
		using yaal::swap_ranges;
		swap_ranges( _coord, _coord + countof ( _coord ), move_._coord );
		swap( _comment, move_._comment );
	}
	return;
	M_EPILOG
}

void SGF::Game::add_stone( Player::player_t player_, int col_, int row_ ) {
	M_PROLOG
	add_stone( player_, Move( col_, row_ ) );
	return;
	M_EPILOG
}

void SGF::Game::add_stone( Player::player_t player_, Move const& move_ ) {
	M_PROLOG
	if ( player_ == Player::BLACK )
		_blackPreset.push_back( move_ );
	else
		_whitePreset.push_back( move_ );
	return;
	M_EPILOG
}

SGF::Game::game_tree_t::node_t SGF::Game::move( game_tree_t::node_t node_, int col_, int row_ ) {
	M_PROLOG
	return ( &*node_->add_node( Move( col_, row_ ) ) );
	M_EPILOG
}

void SGF::Game::clear( void ) {
	M_PROLOG
	_blackPreset.clear();
	_whitePreset.clear();
	_tree.clear();
	_firstToMove = Player::UNSET;
	_blackName.clear();
	_blackRank.clear();
	_whiteName.clear();
	_whiteRank.clear();
	_komi = 5.5;
	_handicap = 0;
	_gobanSize = 19;
	_result = 0;
	_place.clear();
	_comment.clear();
	return;
	M_EPILOG
}

void banner( void ) {
	::printf( "\tSGF\n" );
	return;
}

}

extern "C"
int sgf_sgf_main( int, char** );
extern "C"
int sgf_sgf_main( int, char** ) {
	static char const dynamicLinkerPath[]
		__attribute__(( __section__(".interp") )) = __DYNAMIC_LINKER__;
	if ( dynamicLinkerPath[ 0 ] ) {
		yaal::hcore::banner( PACKAGE_NAME, PACKAGE_VERSION );
		yaal::tools::banner();
		sgf::banner();
		::exit( 0 );
	}
	return ( 0 );
}

