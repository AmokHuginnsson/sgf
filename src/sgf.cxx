/*
---           `sgf' 0.0.0 (c) 1978 by Marcin 'Amok' Konarski            ---

	sgf.cxx - this file is integral part of `sgf' project.

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

#include <cctype>

#include <yaal/hcore/hfile.hxx>
M_VCSID( "$Id: "__ID__" $" )
#include "sgf.hxx"

using namespace yaal;
using namespace yaal::hcore;

namespace sgf
{

char const _errMsg_[][50] =
	{
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
	_currentMove( NULL ), _app( app_ )
	{
	}

char const* non_space( char const* first, char const* last )
	{
	for ( ; first != last ; ++ first )
		{
		if ( ! ::memchr( _whiteSpace_.data(), *first, _whiteSpace_.size() ) )
			break;
		}
	return ( first );
	}

void SGF::load( HStreamInterface& stream_ )
	{
	M_PROLOG
	static int const BUFFER_SIZE( 4096 );
	HChunk buffer( BUFFER_SIZE );
	int nRead( 0 );
	clear();
	while ( ( nRead = static_cast<int>( stream_.read( buffer.raw(), BUFFER_SIZE ) ) ) > 0 )
		_rawData.append( buffer.raw(), nRead );
	_beg = _rawData.begin();
	_cur = _rawData.begin();
	_end = _rawData.end();
	clog << _rawData << endl;
	_cur = non_space( _cur, _end );
	try
		{
		parse_game_tree();
		while ( _cur != _end )
			parse_game_tree();
		}
	catch ( SGFException const& e )
		{
		if ( _cur != _end )
			cerr << "Failed at byte: `" << *_cur << "'" << endl;
		throw;
		}
	return;
	M_EPILOG
	}

void SGF::clear( void )
	{
	M_PROLOG
	_beg = _cur = _end = NULL;
	_currentMove = NULL;
	_rawData.clear();
	_game.clear();
	return;
	M_EPILOG
	}

void SGF::not_eof( void )
	{
	if ( _cur == _end )
		throw SGFException( _errMsg_[ERROR::UNEXPECTED_EOF], static_cast<int>( _cur - _beg ) );
	return;
	}

void SGF::parse_game_tree( void )
	{
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
	while ( ( _cur != _end ) && ( *_cur != TERM::GT_CLOSE ) )
		{
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

void SGF::parse_sequence( void )
	{
	M_PROLOG
	parse_node();
	_cur = non_space( _cur, _end );
	not_eof();
	while ( *_cur == TERM::NODE_MARK )
		{
		_currentMove = &*_currentMove->add_node();
		parse_node();
		_cur = non_space( _cur, _end );
		not_eof();
		}
	return;
	M_EPILOG
	}

void SGF::parse_node( void )
	{
	M_PROLOG
	if ( *_cur != TERM::NODE_MARK )
		throw SGFException( _errMsg_[ERROR::NODE_MARK_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	not_eof();
	while ( ( *_cur != TERM::GT_CLOSE ) && ( *_cur != TERM::GT_OPEN ) && ( *_cur != TERM::NODE_MARK ) )
		{
		parse_property();
		_cur = non_space( _cur, _end );
		not_eof();
		}
	return;
	M_EPILOG
	}

void SGF::parse_property( void )
	{
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
	if ( _cachePropIdent == "GM" )
		{
		if ( lexical_cast<int>( singleValue ) != _gameType )
			throw SGFException( _errMsg_[ERROR::BAD_GAME_TYPE], static_cast<int>( _cur - _beg ) );
		}
	else if ( _cachePropIdent == "FF" )
		{
		int ff( lexical_cast<int>( singleValue ) );
		if ( ( ff < 1 ) || ( ff > 4 ) )
			throw SGFException( _errMsg_[ERROR::BAD_FILE_FORMAT], static_cast<int>( _cur - _beg ) );
		}
	else if ( _cachePropIdent == "PB" )
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
	else if ( _cachePropIdent == "RE" )
		{
		if ( isdigit( singleValue[2] ) )
			_game._result = lexical_cast<int>( singleValue.raw() + 2 );
		else
			{
			char r( static_cast<char>( toupper( singleValue[2] ) ) );
			if ( r == 'R' )
				_game._result = Game::RESIGN;
			else if ( r == 'T' )
				_game._result = Game::TIME;
			}
		char player( static_cast<char>( toupper( singleValue[0] ) ) );
		if ( player == 'W' )
			_game._result = -_game._result;
		}
	else if ( _cachePropIdent == "AB" )
		{
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it )
			_game.add_stone( Player::BLACK, Move( *it ) );
		}
	else if ( _cachePropIdent == "AW" )
		{
		for ( prop_values_t::const_iterator it( _cachePropValue.begin() ), end( _cachePropValue.end() ); it != end; ++ it )
			_game.add_stone( Player::WHITE, Move( *it ) );
		}
	else if ( _cachePropIdent == "B" )
		{
		if ( _game._firstToMove == Player::UNSET )
			_game._firstToMove = Player::BLACK;
		(**_currentMove).coord( singleValue );
		}
	else if ( _cachePropIdent == "W" )
		{
		if ( _game._firstToMove == Player::UNSET )
			_game._firstToMove = Player::WHITE;
		(**_currentMove).coord( singleValue );
		}
	else if ( _cachePropIdent == "C" )
		{
		if ( _game._firstToMove != Player::UNSET )
			(**_currentMove)._comment = singleValue;
		else
			_game._comment += singleValue;
		}
	else
		clog << "property: `" << _cachePropIdent << "' = `" << singleValue << "'" << endl;
	return;
	M_EPILOG
	}

HString const& SGF::parse_property_ident( void )
	{
	M_PROLOG
	_cache.clear();
	while ( ( _cur != _end ) && isupper( *_cur ) )
		{
		_cache += *_cur;
		++ _cur;
		}
	return ( _cache );
	M_EPILOG
	}

void SGF::parse_property_value( prop_values_t& values_ )
	{
	M_PROLOG
	not_eof();
	if ( *_cur != TERM::PROP_VAL_OPEN )
		throw SGFException( _errMsg_[ERROR::PROP_VAL_OPEN_EXPECTED], static_cast<int>( _cur - _beg ) );
	_cur = non_space( ++ _cur, _end );
	not_eof();
	_cache.clear();
	bool escaped( false );
	while ( ( _cur != _end ) && ( escaped || ( *_cur != TERM::PROP_VAL_CLOSE ) ) )
		{
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

void SGF::save( HStreamInterface& stream_ )
	{
	M_PROLOG
	stream_ << "(;GM[" << static_cast<int>( _gameType ) << "]FF[4]AP[" << _app << "]\n"
		<< "SZ[" << _game._gobanSize << "]KM[" << setw( 1 ) << _game._komi << "]TM[" << _game._time << "]\n"
		<< "PB[" << _game._blackName << "]PW[" << _game._whiteName << "]\n"
		<< "BR[" << _game._blackRank << "]WR[" << _game._whiteRank << "]\n";
	if ( ! _game._comment.is_empty() )
		{
		_cache = _game._comment;
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
		}
	if ( ! _game._blackPreset.empty() )
		{
		stream_ << "AB";
		for ( Game::preset_t::const_iterator it( _game._blackPreset.begin() ), end( _game._blackPreset.end() ); it != end; ++ it )
			stream_ << '[' << it->coord() << ']';
		}
	if ( ! _game._whitePreset.empty() )
		{
		stream_ << "AW";
		for ( Game::preset_t::const_iterator it( _game._whitePreset.begin() ), end( _game._whitePreset.end() ); it != end; ++ it )
			stream_ << '[' << it->coord() << ']';
		}
	save_variations( _game._firstToMove, _game._tree.get_root(), stream_ );
	stream_ << ")" << endl;
	return;
	M_EPILOG
	}

void SGF::save_move( Player::player_t of_, Game::game_tree_t::const_node_t node_, HStreamInterface& stream_ )
	{
	M_PROLOG
	stream_ << ';' << ( of_ == Player::BLACK ? 'B' : 'W' ) << '[' << (**node_).coord() << ']';
	if ( ! (**node_)._comment.is_empty() )
		{
		_cache = (**node_)._comment;
		_cache.replace( "[", "\\[" ).replace( "]", "\\]" );
		stream_ << "C[" << _cache << "]";
		}
	return;
	M_EPILOG
	}

void SGF::save_variations( Player::player_t from_, Game::game_tree_t::const_node_t node_, HStreamInterface& stream_ )
	{
	M_PROLOG
	int childCount( 0 );
	while ( ( childCount = static_cast<int>( node_->child_count() ) ) == 1 )
		{
		node_ = &*node_->begin();
		save_move( from_, node_, stream_ );
		from_ = ( from_ == Player::BLACK ? Player::WHITE : Player::BLACK );
		}
	if ( childCount > 1 ) /* We have variations. */
		{
		for ( Game::game_tree_t::const_iterator it( node_->begin() ), end( node_->end() ); it != end; ++ it )
			{
			stream_ << endl << '(';
			save_move( from_, &*it, stream_ );
			save_variations( ( from_ == Player::BLACK ? Player::WHITE : Player::BLACK ), &*it, stream_ );
			stream_ << ')' << endl;
			}
		}
	return;
	M_EPILOG
	}

void SGF::add_stone( Player::player_t player_, int col_, int row_ )
	{
	M_PROLOG
	_game.add_stone( player_, col_, row_ );
	return;
	M_EPILOG
	}

SGF::Game::game_tree_t::node_t SGF::move( Game::game_tree_t::node_t node_, Player::player_t player_, int col_, int row_ )
	{
	M_PROLOG
	return ( _game.move( node_, player_, col_, row_ ) );
	M_EPILOG
	}

void SGF::Game::add_stone( Player::player_t player_, int col_, int row_ )
	{
	M_PROLOG
	add_stone( player_, Move( col_, row_ ) );
	return;
	M_EPILOG
	}

void SGF::Game::add_stone( Player::player_t player_, Move const& move_ )
	{
	M_PROLOG
	if ( player_ == Player::BLACK )
		_blackPreset.push_back( move_ );
	else
		_whitePreset.push_back( move_ );
	return;
	M_EPILOG
	}

SGF::Game::game_tree_t::node_t SGF::Game::move( game_tree_t::node_t node_, Player::player_t, int col_, int row_ )
	{
	M_PROLOG
	return ( &*node_->add_node( Move( col_, row_ ) ) );
	M_EPILOG
	}

void SGF::Game::clear( void )
	{
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

}

