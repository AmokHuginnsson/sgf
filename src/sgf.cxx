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
#include <yaal/tools/assign.hxx>
M_VCSID( "$Id: "__ID__" $" )
#include "sgf.hxx"
#include "config.hxx"

using namespace yaal;
using namespace yaal::hcore;
using namespace yaal::tools;

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
	"Bad file format.",
	"Cannot mix `move' with `setup' nodes.",
	"Duplicated coordinate in setup.",
	"Move not from this record.",
	"Malformed label."
};

SGF::SGF( GAME_TYPE::game_type_t gameType_, HString const& app_ )
	: _gameType( gameType_ ), _rawData(), _beg( NULL ), _cur( NULL ), _end( NULL ),
	_cache(), _cachePropIdent(), _cachePropValue(),
	_currentMove( NULL ), _app( app_ ), _gameName(), _date(),
	_event(), _round(), _source(), _author(), _rules( "Japanese" ),
	_blackName(), _whiteName(), _blackRank( "30k" ), _whiteRank( "30k" ),
	_setups(), _tree(), _firstToMove( Player::UNSET ),
	_gobanSize( 19 ), _time( 0 ), _handicap( 0 ), _komi( 5.5 ),
	_result( 0 ), _place(), _comment() {
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
		swap( _currentMove, sgf_._currentMove );
		swap( _app, sgf_._app );
		swap( _gameName, sgf_._gameName );
		swap( _date, sgf_._date );
		swap( _event, sgf_._event );
		swap( _round, sgf_._round );
		swap( _source, sgf_._source );
		swap( _author, sgf_._author );
		swap( _rules, sgf_._rules );
		swap( _blackName, sgf_._blackName );
		swap( _whiteName, sgf_._whiteName );
		swap( _blackRank, sgf_._blackRank );
		swap( _whiteRank, sgf_._whiteRank );
		swap( _setups, sgf_._setups );
		swap( _tree, sgf_._tree );
		swap( _firstToMove, sgf_._firstToMove );
		swap( _gobanSize, sgf_._gobanSize );
		swap( _time, sgf_._time );
		swap( _handicap, sgf_._handicap );
		swap( _komi, sgf_._komi );
		swap( _result, sgf_._result );
		swap( _place, sgf_._place );
		swap( _comment, sgf_._comment );
	}
	return;
	M_EPILOG
}

void SGF::move( Coord const& coord_ ) {
	M_PROLOG
	if ( ! _currentMove )
		_currentMove = _tree.create_new_root();
	_currentMove = &*_currentMove->add_node( Move( coord_ ) );
	return;
	M_EPILOG
}

void SGF::set_player( Player::player_t player_, yaal::hcore::HString const& name_, yaal::hcore::HString const& rank_ ) {
	M_PROLOG
	if ( player_ == Player::BLACK ) {
		_blackName = name_;
		_blackRank = rank_;
	} else {
		_whiteName = name_;
		_whiteRank = rank_;
	}
	return;
	M_EPILOG
}

void SGF::set_info( Player::player_t player_, int gobanSize_, int handicap_, double komi_, int time_, yaal::hcore::HString const& place_ ) {
	M_PROLOG
	_firstToMove = player_;
	set_board_size( gobanSize_ );
	set_handicap( handicap_ );
	set_komi( komi_ );
	set_time( time_ );
	_place = place_;
	return;
	M_EPILOG
}

void SGF::set_board_size( int gobanSize_ ) {
	M_PROLOG
	_gobanSize = gobanSize_;
	return;
	M_EPILOG
}

void SGF::set_komi( double komi_ ) {
	M_PROLOG
	_komi = komi_;
	return;
	M_EPILOG
}

void SGF::set_handicap( int handicap_ ) {
	M_PROLOG
	_handicap = handicap_;
	return;
	M_EPILOG
}

void SGF::set_time( int time_ ) {
	M_PROLOG
	_time = time_;
	return;
	M_EPILOG
}

void SGF::add_comment( yaal::hcore::HString const& comment_ ) {
	M_PROLOG
	_comment += comment_;
	return;
	M_EPILOG
}

int SGF::get_board_size( void ) const {
	return ( _gobanSize );
}

double SGF::get_komi( void ) const {
	return ( _komi );
}

int SGF::get_handicap( void ) const {
	return ( _handicap );
}

int SGF::get_time( void ) const {
	return ( _time );
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
		_rawData.append( buffer.get<char>(), nRead );
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
			cerr << "Failed at byte: " << ( _cur - _beg ) << " (`" << *_cur << "')" << endl;
		throw;
	}
	return;
	M_EPILOG
}

void SGF::clear( void ) {
	M_PROLOG
	_rawData.clear();
	_beg = _cur = _end = NULL;
	_cache.clear();
	_cachePropIdent.clear();
	_cachePropValue.clear();
	_currentMove = NULL;
	_app.clear();
	_gameName.clear();
	_date.clear();
	_event.clear();
	_round.clear();
	_source.clear();
	_author.clear();
	_rules.clear();
	_blackName.clear();
	_blackRank.clear();
	_whiteName.clear();
	_whiteRank.clear();
	_setups.clear();
	_tree.clear();
	_firstToMove = Player::UNSET;
	_gobanSize = 19;
	_time = 0;
	_handicap = 0;
	_komi = 5.5;
	_result = 0;
	_place.clear();
	_comment.clear();
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
		_currentMove = _tree.create_new_root();
	parse_sequence();
	game_tree_t::node_t preVariationMove( _currentMove );
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
	if ( _currentMove ) {
		_cur = non_space( _cur, _end );
		not_eof();
		while ( *_cur == TERM::NODE_MARK ) {
			if ( ( _currentMove == _tree.get_root() ) || ( (*_currentMove)->type() != Move::TYPE::INVALID ) )
				_currentMove = &*_currentMove->add_node();
			else {
				_cur = non_space( _cur, _end );
				not_eof();
				clog << "Empty node!" << endl;
			}
			parse_node();
			_cur = non_space( _cur, _end );
			not_eof();
		}
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

namespace {

typedef HMap<HString, SGF::Position::position_t> position_tag_dict_t;

position_tag_dict_t const _positionTagDict_ = sequence<HString>( "AE", SGF::Position::REMOVE )
	( "AB", SGF::Position::BLACK )
	( "AW", SGF::Position::WHITE )
	( "TR", SGF::Position::TRIANGLE )
	( "SQ", SGF::Position::SQUARE )
	( "CR", SGF::Position::CIRCLE )
	( "MA", SGF::Position::MARK )
	( "TB", SGF::Position::BLACK_TERITORY )
	( "TW", SGF::Position::WHITE_TERITORY );

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
	} else if ( _cachePropIdent == "AP" )
		_app = singleValue;
	else if ( _cachePropIdent == "GN" )
		_gameName = singleValue;
	else if ( _cachePropIdent == "DT" )
		_date = singleValue;
	else if ( _cachePropIdent == "EV" )
		_event = singleValue;
	else if ( _cachePropIdent == "RO" )
		_round = singleValue;
	else if ( _cachePropIdent == "SO" )
		_source = singleValue;
	else if ( _cachePropIdent == "AN" )
		_author = singleValue;
	else if ( _cachePropIdent == "RU" )
		_rules = singleValue;
	else if ( _cachePropIdent == "PB" )
		_blackName = singleValue;
	else if ( _cachePropIdent == "PW" )
		_whiteName = singleValue;
	else if ( _cachePropIdent == "BR" )
		_blackRank = singleValue;
	else if ( _cachePropIdent == "WR" )
		_whiteRank = singleValue;
	else if ( _cachePropIdent == "KM" )
		_komi = lexical_cast<double>( singleValue );
	else if ( _cachePropIdent == "HA" )
		_handicap = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "SZ" )
		_gobanSize = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "TM" )
		_time = lexical_cast<int>( singleValue );
	else if ( _cachePropIdent == "PC" )
		_place = singleValue;
	else if ( _cachePropIdent == "RE" ) {
		if ( isdigit( singleValue[2] ) )
			_result = lexical_cast<int>( singleValue.raw() + 2 );
		else {
			char r( static_cast<char>( toupper( singleValue[2] ) ) );
			if ( r == 'R' )
				_result = RESIGN;
			else if ( r == 'T' )
				_result = TIME;
		}
		char player( static_cast<char>( toupper( singleValue[0] ) ) );
		if ( player == 'W' )
			_result = _result;
	} else if ( _positionTagDict_.find( _cachePropIdent ) != _positionTagDict_.end() ) {
		position_tag_dict_t::const_iterator tag( _positionTagDict_.find( _cachePropIdent ) );
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it )
			add_position( tag->second, Coord( *it ) );
	} else if ( _cachePropIdent == "LB" ) {
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it ) {
			if ( it->find( ":" ) != 2 )
				throw SGFException( _errMsg_[ERROR::MALFORMED_LABEL], static_cast<int>( _cur - _beg ) );
			_cache.assign( it->raw() + 3, it->get_length() - 3 );
			add_label( make_pair( Coord( *it ), _cache ) );
		}
	} else if ( _cachePropIdent == "B" ) {
		if ( _firstToMove == Player::UNSET )
			_firstToMove = Player::BLACK;
		(*_currentMove)->set_coord( singleValue );
	} else if ( _cachePropIdent == "W" ) {
		if ( _firstToMove == Player::UNSET )
			_firstToMove = Player::WHITE;
		(*_currentMove)->set_coord( singleValue );
	} else if ( _cachePropIdent == "C" ) {
		if ( _firstToMove != Player::UNSET )
			(*_currentMove)->add_comment( singleValue );
		else
			_comment += singleValue;
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
		<< "RU[" << _rules << "]" << "SZ[" << _gobanSize << "]";
	if ( _handicap > 0 )
		stream_ << "HA[" << _handicap << "]";
	stream_ << "KM[" << setw( 1 ) << _komi << "]";
	if ( ! _gameName.is_empty() )
		stream_ << "GN[" << _gameName << "]";
	stream_ << "TM[" << _time << ( noNL_ ? "]" : "]\n" )
		<< "PB[" << _blackName << "]PW[" << _whiteName << "]";
	if ( ! _blackRank.is_empty() )
		stream_ << ( noNL_ ? "" : "\n" ) << "BR[" << _blackRank << "]";
	if ( ! _whiteRank.is_empty() )
		stream_ << "WR[" << _whiteRank << "]";
	if ( ! noNL_ && ! ( _blackRank.is_empty() && _whiteRank.is_empty() ) )
		stream_ << "\n";
	if ( ! _date.is_empty() )
		stream_ << "DT[" << _date << "]";
	if ( ! _event.is_empty() )
		stream_ << "EV[" << _event << "]";
	if ( ! _round.is_empty() )
		stream_ << "RO[" << _round << "]";
	if ( ! _source.is_empty() )
		stream_ << "SO[" << _source << "]";
	if ( ! _author.is_empty() )
		stream_ << "AN[" << _author << "]";
	if ( ! _comment.is_empty() ) {
		_cache = _comment;
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
	}
	if ( ! _tree.is_empty() ) {
		game_tree_t::const_node_t root( _tree.get_root() );
		if ( (*root)->setup() )
			save_setup( root, stream_, noNL_ );
		save_variations( _firstToMove, root, stream_, noNL_ );
	}
	stream_ << ( noNL_ ? ")" : ")\n" );
	return;
	M_EPILOG
}

void SGF::save_setup( game_tree_t::const_node_t node_, yaal::hcore::HStreamInterface& stream_, bool noNL_ ) {
	M_PROLOG
	char const* setupTag[] = {
		"AE", "AB", "AW", "TR", "SQ", "CR", "MA", "TB", "TW"
	};
	Move const& m( *(*node_) );
	Setup const& setup( *m.setup() );
	if ( ( m.type() == Move::TYPE::SETUP ) && ( node_ != _tree.get_root() ) )
		stream_ << ( noNL_ ? ";" : "\n;" );
	Setup::setup_t::const_iterator toRemove( setup._data.find( Position::REMOVE ) );
	if ( toRemove != setup._data.end() ) {
		stream_ << "AE";
		for ( Setup::coords_t::const_iterator it( toRemove->second.begin() ), end( toRemove->second.end() ); it != end; ++ it )
			stream_ << "[" << it->data() << "]";
	}
	for ( Setup::setup_t::const_iterator it( setup._data.begin() ), end( setup._data.end() ); it != end; ++ it ) {
		if ( it->first == Position::REMOVE ) {
			continue;
		} else {
			stream_ << setupTag[it->first];
			for ( Setup::coords_t::const_iterator c( it->second.begin() ), ce( it->second.end() ); c != ce; ++ c )
				stream_ << "[" << c->data() << "]";
		}
	}
	if ( ! setup._labels.is_empty() ) {
		stream_ << "LB";
		for ( Setup::labels_t::const_iterator it( setup._labels.begin() ), end( setup._labels.end() ); it != end; ++ it ) {
			stream_ << "[" << it->first.data() << ":" << it->second << "]";
		}
	}
	return;
	M_EPILOG
}

void SGF::save_move( Player::player_t of_, game_tree_t::const_node_t node_, HStreamInterface& stream_, bool noNL_ ) {
	M_PROLOG
	stream_ << ( noNL_ ? ";" : "\n;" ) << ( of_ == Player::BLACK ? 'B' : 'W' ) << '[' << (*node_)->coord().data() << ']';
	if ( ! (*node_)->comment().is_empty() ) {
		_cache = (*node_)->comment();
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
	}
	return;
	M_EPILOG
}

void SGF::save_variations( Player::player_t from_, game_tree_t::const_node_t node_, HStreamInterface& stream_, bool noNL_ ) {
	M_PROLOG
	int childCount( 0 );
	while ( ( childCount = static_cast<int>( node_->child_count() ) ) == 1 ) {
		node_ = &*node_->begin();
		if ( (*node_)->type() == Move::TYPE::MOVE ) {
			save_move( from_, node_, stream_, noNL_ );
			from_ = ( from_ == Player::BLACK ? Player::WHITE : Player::BLACK );
		}
		if ( (*node_)->setup() )
			save_setup( node_, stream_, noNL_ );
	}
	if ( childCount > 1 ) /* We have variations. */ {
		for ( game_tree_t::HNode::const_iterator it( node_->begin() ), end( node_->end() ); it != end; ++ it ) {
			stream_ << ( noNL_ ? "(" : "\n(" );
			Player::player_t p( from_ );
			if ( (*it)->type() == Move::TYPE::MOVE ) {
				save_move( from_, &*it, stream_, noNL_ );
				p = ( p == Player::BLACK ? Player::WHITE : Player::BLACK );
			}
			if ( (*it)->setup() )
				save_setup( &*it, stream_, noNL_ );
			save_variations( p, &*it, stream_, noNL_ );
			stream_ << ( noNL_ ? ")" : ")\n" );
		}
	}
	return;
	M_EPILOG
}

void SGF::Move::swap( Move& move_ ) {
	M_PROLOG
	if ( &move_ != this ) {
		using yaal::swap;
		swap( _type, move_._type );
		swap( _coord, move_._coord );
		swap( _comment, move_._comment );
		swap( _setup, move_._setup );
	}
	return;
	M_EPILOG
}

void SGF::Move::set_coord( Coord const& coord_ ) {
	M_PROLOG
	if ( _type == TYPE::SETUP )
		throw SGFException( _errMsg_[ERROR::MIXED_NODE] );
	_coord = coord_;
	_type = TYPE::MOVE;
	return;
	M_EPILOG
}

void SGF::add_position( Position::position_t position_, Coord const& coord_ ) {
	M_PROLOG
	if ( ! _currentMove ) {
		_setups.push_back( Setup() );
		_currentMove = _tree.create_new_root( Move( &_setups.back() ) );
	} else if ( ! (*_currentMove)->setup() ) {
		_setups.push_back( Setup() );
		(*_currentMove)->set_setup( &_setups.back() );
	}
	(*_currentMove)->add_position( position_, coord_ );
	return;
	M_EPILOG
}

void SGF::add_label( Setup::label_t const& label_ ) {
	M_PROLOG
	if ( ! _currentMove ) {
		_setups.push_back( Setup() );
		_currentMove = _tree.create_new_root( Move( &_setups.back() ) );
	} else if ( ! (*_currentMove)->setup() ) {
		_setups.push_back( Setup() );
		(*_currentMove)->set_setup( &_setups.back() );
	}
	(*_currentMove)->add_label( label_ );
	return;
	M_EPILOG
}

SGF::game_tree_t::node_t SGF::move( game_tree_t::node_t node_, Coord const& coord_ ) {
	M_PROLOG
	return ( &*node_->add_node( Move( coord_ ) ) );
	M_EPILOG
}

void SGF::Move::add_position( Position::position_t position_, Coord const& coord_ ) {
	M_PROLOG
	if ( ( position_ == Position::REMOVE )
			|| ( position_ == Position::BLACK )
			|| ( position_ == Position::WHITE ) ) {
		if ( _type == TYPE::MOVE )
			throw SGFException( _errMsg_[ERROR::MIXED_NODE] );
		_type = TYPE::SETUP;
	}
	_setup->add_position( position_, coord_ );
	M_EPILOG
}

void SGF::Move::add_label( Setup::label_t const& label_ ) {
	M_PROLOG
	_setup->add_label( label_ );
	M_EPILOG
}

void SGF::Move::add_comment( yaal::hcore::HString const& comment_ ) {
	M_PROLOG
	_comment += comment_;
	return;
	M_EPILOG
}

void SGF::Move::set_setup( Setup* setup_ ) {
	M_PROLOG
	M_ASSERT( ! _setup );
	_setup = setup_;
	return;
	M_EPILOG
}

void SGF::Setup::add_position( Position::position_t position_, Coord const& coord_ ) {
	M_PROLOG
	if ( position_ == Position::REMOVE ) {
		for ( Setup::setup_t::iterator it( _data.begin() ), end( _data.end() ); it != end; ++ it ) {
			if ( it->first == Position::REMOVE )
				continue;
			it->second.remove( coord_ );
		}
	} else {
		for ( Setup::setup_t::iterator it( _data.begin() ), end( _data.end() ); it != end; ++ it ) {
			if ( it->first == Position::REMOVE )
				continue;
			if ( find( it->second.begin(), it->second.end(), coord_ ) != it->second.end() )
				throw SGFException( _errMsg_[ERROR::DUPLICATED_COORDINATE] );
		}
	}
	coords_t& c( _data[position_] );
	if ( ( position_ != Position::REMOVE ) || ( find( c.begin(), c.end(), coord_ ) == c.end() ) )
		c.push_back( coord_ );
	return;
	M_EPILOG
}

void SGF::Setup::add_label( label_t const& label_ ) {
	M_PROLOG
	_labels.push_back( label_ );
	return;
	M_EPILOG
}

SGF::game_tree_t const& SGF::game_tree( void ) const {
	return ( _tree );
}

SGF::game_tree_t::const_node_t SGF::get_current_move( void ) const {
	return ( _currentMove );
}

void SGF::set_current_move( game_tree_t::const_node_t node_ ) {
	M_PROLOG
	if ( &node_->get_tree() != &_tree )
		throw SGFException( _errMsg_[ERROR::MOVE_OUT_OF_RECORD] );
	_currentMove = const_cast<game_tree_t::node_t>( node_ );
	return;
	M_EPILOG
}

void SGF::Move::clear_markers( Position::position_t position_ ) {
	M_PROLOG
	if ( _setup )
		_setup->_data.erase( position_ );
	return;
	M_EPILOG
}

void SGF::clear_markers( game_tree_t::const_node_t node_ ) {
	M_PROLOG
	if ( &node_->get_tree() != &_tree )
		throw SGFException( _errMsg_[ERROR::MOVE_OUT_OF_RECORD] );
	game_tree_t::node_t node( const_cast<game_tree_t::node_t>( node_ ) );
	(*node)->clear_markers( Position::BLACK_TERITORY );
	(*node)->clear_markers( Position::WHITE_TERITORY );
	(*node)->clear_markers( Position::SQUARE );
	(*node)->clear_markers( Position::CIRCLE );
	(*node)->clear_markers( Position::TRIANGLE );
	(*node)->clear_markers( Position::MARK );
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

