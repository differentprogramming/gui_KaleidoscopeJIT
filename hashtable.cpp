#include "..\interactive\stdafx.h"



void LexToken::get_text(char * buf, int buflen)
{
	int l;
	if (len > buflen-1) l = buflen - 1;
	else l = len;

	if (value_tag == TVT_STRING && clean_up_data != nullptr) {
		const char *err;
		int destlen;
		const char *cleaned = (*clean_up_data)(destlen, buf, err, &parent->source[name], l);
		if (err != nullptr) {
			std::ostringstream s;
			memcpy(buf, &parent->source[name], l);
			buf[l] = 0;
			s << " warning " << err << " in {{" << buf << "}} ";
			delete[] cleaned;
			throw std::invalid_argument(s.str());
		}
		return;
	}

	memcpy(buf, &parent->source[name], l);
	buf[l] = 0;
}

const char * LexToken::get_text()
{
	const char *c = new char[len + 1];
	get_text(const_cast<char *>(c), len + 1);
	return c;
}

 int16_t SolidAsciiTokenizer::char_classes[256];
//if _seed isn't 0, then assume you've precomputed a seed that works.
PerfectHashTable::PerfectHashTable(const char **keywords, int num_keywords, uint64_t _seed , int known_size ) :
	rnd(_seed^0x4b45aa56c384190eull, known_size != 0),
	total_tokens(num_keywords),
	seed(_seed),
	names(keywords)
{	
	//if the number of keywords wasn't already passed in, then assume a nullptr terminated list and count the keywords
	if (num_keywords == 0)
	{
		while (keywords[num_keywords] != nullptr) ++num_keywords;
		total_tokens = num_keywords;
	}
	//helper function to throw duplicate keyword
	auto throw_err = [&](int i) {
		std::string err_str = "duplicate keyword \"";
		err_str += keywords[i];
		err_str += "\"";
		throw(std::invalid_argument(err_str));
	};
	//token number -> string length
	keylen.resize(num_keywords, 0);
	
	//by_length will be an array of token numbers, ordered by length from high to low
	by_length.resize(num_keywords);
	for (int i = 0; i < num_keywords; ++i) {
		by_length[i] = i;
		keylen[i] = (int)strlen(keywords[i]);
	}
	std::sort(by_length.begin(), by_length.end(), [&](int i, int j) { return keylen[i] > keylen[j]; });

	//length index will be token_number -> position by length, for instance the longest token will have index 0
	length_index.resize(num_keywords);
	for (int i = 0; i < num_keywords; ++i) length_index[by_length[i]] = i;

	//number_of_token_in_hash counts the number of tokens not in the single char array or the double char array
	int number_of_token_in_hash = 0;
	for (int i = 0; i < 256; ++i) {
		max_token_length[i] = 0;
		single_char_tokens[i] = -1;
	}
	
	for (int i = sizeof(double_char_tokens) / sizeof(double_char_tokens[0]) - 1; i >= 0; --i) double_char_tokens[i] = -1;
	for (int i = 0; i < num_keywords; ++i) {
		int l_down = length_index[i];
		unsigned char c = keywords[l_down][0];
		if (max_token_length[c] < keylen[l_down]) max_token_length[c] = keylen[l_down];
		int len = keylen[i];
		int double_index;
		switch (len) {
		case 0: throw(std::invalid_argument("zero length keyword"));
		case 1:
			if (single_char_tokens[(unsigned char)keywords[i][0]] != -1) throw_err(i);
			single_char_tokens[(unsigned char)keywords[i][0]] = i;
			break;
		case 2:
			if (calc_double_char_index(double_index, keywords[i])) {
				if (double_char_tokens[double_index] != -1) throw_err(i);
				double_char_tokens[double_index] = i;
				break;
			}
		default:
			++number_of_token_in_hash;
		}
	}

	auto set_sizes = [&]() {
		entries.resize(HASH_SIZE << 1, 0);
		token_number_per_entry.resize(HASH_SIZE, -1);
	};
	if (known_size == 0 && number_of_token_in_hash > 0) {
		HASH_SIZE = 1 << high_bit_number(number_of_token_in_hash * 3);
		do {
			set_sizes();
			for (int hashtries = 0; hashtries < 100000;  ++hashtries) {
				seed = rnd.random_value();
				for (int i = 0; i < num_keywords; ++i) {
					if (keylen[i] > 2) {
						uint64_t h1 = seed ^ keylen[i];
						uint64_t h2 = h1;
						spooky_hash128(keywords[i], keylen[i], &h1, &h2);
						int index = (int)(h1&(HASH_SIZE - 1));
						if (token_number_per_entry[index] != -1) {
							if (0 == strncmp(keywords[token_number_per_entry[index]], keywords[i], MAX_HASH_STRING_SIZE)) throw_err(i);
							goto continue2;
						}
						token_number_per_entry[index] = i;
						index <<= 1;
						entries[index++] = h1;
						entries[index] = h2;
					}
				}
				goto success;
			continue2:
				std::fill(entries.begin(), entries.end(), 0);
				std::fill(token_number_per_entry.begin(), token_number_per_entry.end(), -1);
			}
			HASH_SIZE <<= 1;
//			printf("Increasing hash table size to %d\n", HASH_SIZE);
		} while (true);//try next size
	success:
		printf("found perfect hash HASH_SIZE = %d total keywords=%d large keywords=%d small=%d seed = %llx\n", HASH_SIZE, total_tokens, number_of_token_in_hash, total_tokens- number_of_token_in_hash, seed);
	}
	else {
		HASH_SIZE = known_size;
		if (number_of_token_in_hash > 0) {
			set_sizes();
			for (int i = 0; i < num_keywords; ++i) {
				if (keylen[i] > 2) {
					uint64_t h1 = seed ^ keylen[i];
					uint64_t h2 = h1;
					spooky_hash128(keywords[i], keylen[i], &h1, &h2);
					int index = (int)(h1&(HASH_SIZE - 1));
					if (token_number_per_entry[index] != -1) throw_err(i);//not exactly the right error but this is only a sanity check
					token_number_per_entry[index] = i;
					index <<= 1;
					entries[index++] = h1;
					entries[index] = h2;
					//std::cout << "hashed " << keywords[i] << " to " << std::setfill('0') << std::setw(16) << std::hex << h1 << std::setfill('0') << std::setw(16) << std::hex << h2 << '\n';
				}
			}
		}
	}
}

bool PerfectHashTable::find(int &token_number, uint64_t &hash, const char *name, int len ) 
{
	int t;
	if (len == 0) len = (int)strlen(name);
	switch (len)
	{
	case 0: throw(std::invalid_argument("zero length token"));
	case 1:
		t = single_char_tokens[(unsigned char)name[0]];
		if (t != -1) {
			token_number = t;
			return true;
		}
		break;

	case 2:
		if (calc_double_char_index(t, name) && double_char_tokens[t] != -1) {
			token_number = double_char_tokens[t];
			return true;
		}
	}
	uint64_t h1 = seed ^ len;
	uint64_t h2 = h1;
	spooky_hash128(name, len, &h1, &h2);
	int index = (int)(h1&(HASH_SIZE - 1));
	t = token_number_per_entry[index];
	index <<= 1;
	if (t == -1 || h1 != entries[index] || h2 != entries[index + 1]) {
		hash = h1;
		return false;
	}
	token_number = t;
	return true;
}

int high_bit_number(int i) {
	int n = -1;
	while (i != 0) {
		i >>= 1;
		++n;
	}
	return n;
}

Random64::Random64(uint64_t seed, bool dont_bother)
{
	//cludge so that we don't waste time if we don't need random numbers
	if (dont_bother) return;
	int i;
	a = 0xdeadbeef;
	b = c = d = seed;

	for (i = 0; i < 20; ++i) {
		random_value();
	}
}
SolidAsciiTokenizer::SolidAsciiTokenizer(bool do_c,const char **keywords, int num_keywords, uint64_t _seed, int known_size) :
	PerfectHashTable(keywords, num_keywords, _seed, known_size),
	number_of_tokens_parsed(0),
	short_token_context_position(0),
	source_pos(0),
	filenames_pos(0),
	line(0),
	line_offset(0),
	do_c(do_c),
	eof(false)

{
	for (int i = 0; i < TOKEN_CONTEXT_LENGTH; ++i) short_token_context[i].parent = this;
	for (int i = 0; i < 256; ++i) {
		char c = (char)i;
		int16_t classes = 0;
		if (c>=0 && isalpha(c)) {
			classes |= CCALPHA;
			if (islower(c)) classes |= CCLOWER;
			else classes |= CCUPPER;
			switch (c)
			{
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'c':
			case 'C':
			case 'd':
			case 'D':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
				classes |= CCHEX;//maybe a comment
				break;
			}
		}
		else if (c >= 0 && isdigit(c)) {
			classes |= CCDIGIT;
			classes |= CCHEX;
		}
		else if (c >= 0 && isspace(c)) {
			classes |= CCSPACE;
		}
		else if (c=='\\') { classes |= CCSPACE; }//for now handle preprocessor splice lines as just space
		else if (c >= 0 && ispunct(c)) {
			switch (c) {
			case '_':
				classes |= CC_;
				break;
			case '"':
				if (!do_c) classes |= CCDQUOTE;
				break;
			case '%':
				if (!do_c) classes |= CCPERCENT;
				break;
			case '@':
				if (!do_c) classes |= CCAT;
			}
		}
		char_classes[i] = classes;
	}
}

							//  0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A  B  C  D  E  F  G H I J K L M N O P Q R S T U V W X Y Z [ / ] ^ _ ` a  b  c  d  e  f
const uint8_t char_to_hex[] = { 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,11,12,13,14,15 };

enum PS_MODES {
	PSM_NORMAL,
	PSM_SLASH,
	PSM_SLASHX,
	PSM_SLASHX1,
	PSM_SLASHu,
	PSM_SLASHu1,
	PSM_SLASHu2,
	PSM_SLASHu3,
	PSM_SLASHU,
	PSM_SLASHU1,
	PSM_SLASHU2,
	PSM_SLASHU3,
	PSM_SLASHU4,
	PSM_SLASHU5,
	PSM_SLASHU6,
	PSM_SLASHU7,
	PSM_SLASHO1,
	PSM_SLASHO2,

};
/*
 * @author Ondrej Hruska <ondra@ondrovo.com>
 * @license MIT
 */
char * utf8_encode(bool &ok, char *out, uint32_t utf)
{
	ok = true;
	if (utf <= 0x7F) {
		// Plain ASCII
		*out++ = (char)utf;
	}
	else if (utf <= 0x07FF) {
		// 2-byte unicode
		*out++ = (char)(((utf >> 6) & 0x1F) | 0xC0);
		*out++ = (char)(((utf >> 0) & 0x3F) | 0x80);
	}
	else if (utf <= 0xFFFF) {
		// 3-byte unicode
		*out++ = (char)(((utf >> 12) & 0x0F) | 0xE0);
		*out++ = (char)(((utf >> 6) & 0x3F) | 0x80);
		*out++ = (char)(((utf >> 0) & 0x3F) | 0x80);
	}
	else if (utf <= 0x10FFFF) {
		// 4-byte unicode
		*out++ = (char)(((utf >> 18) & 0x07) | 0xF0);
		*out++ = (char)(((utf >> 12) & 0x3F) | 0x80);
		*out++ = (char)(((utf >> 6) & 0x3F) | 0x80);
		*out++ = (char)(((utf >> 0) & 0x3F) | 0x80);
	}
	else {
		// error - use replacement character
		*out++ = (char)0xEF;
		*out++ = (char)0xBF;
		*out++ = (char)0xBD;
		ok = false;
	}
	return out;
}


//Process C string with escapes into ascii, however unicode points are processed into utf8.
//skips over \r so that Unix and Windows input will be processed the to the same result
//Also, errors in the string just produce a warning, it doesn't throw an error.
const char *process_string(int &destlen,char*dest,const char *&err, const char *source, int len)
{
	const char *only_error ="error in escape sequence";
	char *r = dest;
	uint32_t v;
	err = nullptr;
	PS_MODES p = PSM_NORMAL;
	for (int i = 0; i < len; ++i) {
		if (p == PSM_NORMAL) {
			if (source[i] != '\\') {
				if (source[i]!='\r') *r++ = source[i];
			}
			else {
				v = 0;
				p = PSM_SLASH;
			}
		}
		else {
			switch (p){
			case PSM_SLASH:
				switch (source[i]) {
				case 'a':
					*r++ = 7; p = PSM_NORMAL; break;
				case 'b':
					*r++ = 8; p = PSM_NORMAL; break;
				case 'e':
					*r++ = 0x1b; p = PSM_NORMAL; break;
				case 'f':
					*r++ = 0xc; p = PSM_NORMAL; break;
				case 'n':
					*r++ = 0xa; p = PSM_NORMAL; break;
				case 'r':
					*r++ = 0xd; p = PSM_NORMAL; break;
				case 't':
					*r++ = 9; p = PSM_NORMAL; break;
				case 'v':
					*r++ = 0xb; p = PSM_NORMAL; break;
				case '\\':
					*r++ = '\\'; p = PSM_NORMAL; break;
				case '\'':
					*r++ = '\''; p = PSM_NORMAL; break;
				case '"':
					*r++ = '"'; p = PSM_NORMAL; break;
				case '?':
					*r++ = '?'; p = PSM_NORMAL; break;
				case 'x':
					p = PSM_SLASHX; break;
				case 'u':
					p = PSM_SLASHu; break;
				case 'U':
					p = PSM_SLASHU; break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					v = source[i] - '0';
					p = PSM_SLASHO1;
					break;
				default:
					err = only_error;
					if (source[i] != '\r') *r++ = source[i];
				}
				break;
			case PSM_SLASHX:
			case PSM_SLASHX1:
				if (source[i] == '0' || (source[i] > '0' && source[i] <= 'f' && char_to_hex[source[i-'0']] != 0)) {
					v <<= 4;
					v += char_to_hex[source[i - '0']];
					p = PSM_SLASHX1;
				}
				else {
					if (p == PSM_SLASHX) {
						err = only_error;
					}
					else *r++ = (char)v;
					p = PSM_NORMAL;
					--i;
				}
				break;
			case PSM_SLASHu:
			case PSM_SLASHu1:
			case PSM_SLASHu2:
			case PSM_SLASHu3:
			case PSM_SLASHU:
			case PSM_SLASHU1:
			case PSM_SLASHU2:
			case PSM_SLASHU3:
			case PSM_SLASHU4:
			case PSM_SLASHU5:
			case PSM_SLASHU6:
			case PSM_SLASHU7:
				if (source[i] == '0' || (source[i] > '0' && source[i] <= 'f' && char_to_hex[source[i - '0']] != 0)) {
					v <<= 4;
					v += char_to_hex[source[i - '0']];
					if (p == PSM_SLASHu3 || p == PSM_SLASHU7) {
						bool ok;
						r = utf8_encode(ok, r, v);
						if (!ok) err = only_error;
						p = PSM_NORMAL;
					}
					else p = (PS_MODES)(p + 1);
				}
				else {
					err = "incomplete unicode point";
					if (p != PSM_SLASHu && p != PSM_SLASHU) {
						bool ok;
						r = utf8_encode(ok, r, v);
					}
					p = PSM_NORMAL;
					--i;
				}
				break;
			case PSM_SLASHO1:
				if (source[i]>='0' && source[i]<='7') v = (v<<3)+source[i] - '0';
				else {
					*r++ = (char)v;
					p = PSM_NORMAL;
					--i;
				}
				break;
			case PSM_SLASHO2:
				if (source[i] >= '0' && source[i] <= '7') v = (v << 3) + source[i] - '0';
				else --i;
				*r++ = (char)v;
				p = PSM_NORMAL;
				break;
			}
		}
	}
	if (p== PSM_SLASHO1 || p== PSM_SLASHO2 || p == PSM_SLASHX1) *r++ = (char)v;
	else if (p == PSM_SLASHu3 || p == PSM_SLASHU7) {
		bool ok;
		r = utf8_encode(ok, r, v);
		if (!ok) err = only_error;
	}
	else if (p!= PSM_NORMAL) err = only_error;
	*r++ = 0;
	destlen = r - dest - 1;
	return dest;
}


bool SolidAsciiTokenizer::tokenize(bool skip_whitespace_and_comments)
{
	const int16_t ID_OPENING_CC = do_c ? (CC_ | CCALPHA ) : (CC_ | CCALPHA | CCPERCENT | CCAT);
	
	do {
		//scan till end
		if (source_pos >= (int)source.size()) return false;
		inc_token();
		LexToken &t = short_token_context[short_token_context_position];

		t.name = source_pos;
		unsigned char c = (unsigned char)source[source_pos++];
		if (c == 0) {
			--source_pos;
			//dec_token();
			eof = true;
			t.token_number = do_c ? CTOK_EOF : TK_EOF;
			t.len = 0;
			t.ignore = false;
			t.filename = filenames_pos;
			t.value_tag = LexToken::TVT_NONE;
			t.clean_up_data = nullptr;
			return false;
		}
		t.line_offset = line_offset;
		t.line = line;
		++line_offset;

		int16_t classes = char_classes[c];

		int len = 0;
		t.len = 0;
		t.ignore = false;
		t.filename = filenames_pos;
		t.value_tag = LexToken::TVT_NONE;
		t.clean_up_data = nullptr;
		//whitespace
		if (0 != (classes & CCSPACE)) {
			do {
				c = (unsigned char)source[source_pos++];
				++line_offset;
				++t.len;
				if (c == '\n') {
					++t.line;
					t.line_offset = 0;
				}
			} while (0 != (char_classes[c] & CCSPACE));
			--source_pos;
			t.ignore = true;
			t.token_number = do_c ? CTOK_WHITESPACE: TK_WHITESPACE;
			if (skip_whitespace_and_comments) continue;
			return true;
		}
		//comments
		if (c == '/') {
			if (source[source_pos] == '/') {
				do {
					c = (unsigned char)source[source_pos++];
					if (c != '\r') ++t.len;
					++line_offset;
				} while (c != '\n' && c != 0);
				if (c == '\n') {
					++t.line;
					t.line_offset = 0;
				}
				else if (c == 0) {
					--source_pos;
				}
				t.ignore = true;
				t.token_number = do_c ? CTOK_COMMENT : TK_COMMENT;
				if (skip_whitespace_and_comments) continue;
				return true;
			}
			else if (source[source_pos] == '*') {
				do {
					c = (unsigned char)source[source_pos++];
					++line_offset;
					++t.len;
					if (c == '\n') {
						++t.line;
						t.line_offset = 0;
					}
				} while (!((c == '*' && source[source_pos] == '/') || c == 0));
				if (c == 0) {
					--source_pos;
				}
				else {
					++t.len;
					++source_pos;
				}
				t.ignore = true;
				t.token_number = do_c? CTOK_COMMENT:TK_COMMENT;
				if (skip_whitespace_and_comments) continue;
				return true;
			}
		}
		//identifiers
		if (0 != (classes & ID_OPENING_CC))
		{
			t.value_tag = LexToken::TVT_ID;
			bool in_at = (0 != (classes & CCAT));
			if (in_at) {
				++t.name;
				c = (unsigned char)source[source_pos++];
				++line_offset;

				classes = char_classes[c];
				if (0 != (classes & (CC_ | CCALPHA | CCPERCENT ))) throw(std::invalid_argument("identifier expected directly after @"));
			}
			if (0 != (classes & CCPERCENT))
			{
				++t.name;
				while (source[source_pos++] != '%') {
					++t.len;
					++t.line_offset;
					if (source[source_pos - 1] == '\n') {
						++t.line;
						t.line_offset = 0;
					}
					if (source[source_pos - 1] == 0) {
						--source_pos;
						t.token_number = -1;
						throw(std::invalid_argument("file ended inside of delimited identifier"));
					}
				}
				if (t.len == 0) {
					t.token_number = -1;
					throw(std::invalid_argument("empty delimited identifier"));
				}
				++t.line_offset;
			}
			else {
				do {
					c = (unsigned char)source[source_pos++];
					++line_offset;
				} while (0 != (char_classes[c] & (CC_ | CCALPHA | CCDIGIT)));
				--source_pos;
				int len = source_pos - t.name;
				if (!do_c && c == '=') {
					switch (len) {
					case 4:
						//looking for band= or bxor=
						if (source[t.name] == 'b') {
							if (source[t.name + 1] == 'a'&&source[t.name + 2] == 'n'&&source[t.name + 3] == 'd') {
								++source_pos;
								++line_offset;
								t.token_number = TK_BANDEQ;
								t.len = len + 1;
								return true;
							}
							if (source[t.name + 1] == 'x'&&source[t.name + 2] == 'o'&&source[t.name + 3] == 'r') {
								++source_pos;
								++line_offset;
								t.token_number = TK_BXOREQ;
								t.len = len + 1;
								return true;
							}
						}
						break;
					case 3:
						//NOTEQ, BOREQ, MODEQ;
						if (source[t.name + 1] == 'o') {
							if (source[t.name] == 'n'&&source[t.name + 2] == 't') {
								++source_pos;
								++line_offset;
								t.token_number = TK_NOTEQ;
								t.len = len + 1;
								return true;
							}
							if (source[t.name] == 'b'&&source[t.name + 2] == 'r') {
								++source_pos;
								++line_offset;
								t.token_number = TK_BOREQ;
								t.len = len + 1;
								return true;
							}
							if (source[t.name] == 'm'&&source[t.name + 2] == 'd') {
								++source_pos;
								++line_offset;
								t.token_number = TK_MODEQ;
								t.len = len + 1;
								return true;
							}
						}
					}
				}
				t.len = len;
				if (max_token_length[(uint8_t)source[t.name]] < len) { //we know it's not a keyword so don't bother looking that up
					uint64_t h2 = t.hash = seed ^ len;
					spooky_hash128(&source[t.name], len, &t.hash, &h2);
				}
				else if (find(t.token_number, t.hash, &source[t.name], len)) return true;
			}
			t.value_tag = LexToken::TVT_STRING;
			t.token_number = do_c?CTOK_IDENT:(in_at?TK_MACROID:TK_IDENT);
			return true;
		}
		//numbers
		if (0 != (classes & CCDIGIT) || (c == '.' && 0 != (char_classes[source[source_pos]] & CCDIGIT))) {
			if (c == '0' && (source[source_pos] == 'x' || source[source_pos] == 'X')) {
				++line_offset;
				t.int_value = 0;
				c = source[++source_pos];
				if (0 == (char_classes[c] & CCHEX)) {
					t.token_number = -1; //error
					--source_pos;
					t.len = 2;
					throw(std::invalid_argument("unfinished hexadecimal number token"));
				}
				do {
					if (0 != (t.int_value & (15ull << 60))) {
						while (0 != (char_classes[source[++source_pos]] & CCHEX)) ++line_offset;;
						t.token_number = -1; //error
						t.len = source_pos - t.name;
						throw(std::invalid_argument("hexadecimal number too large to fit in 8 bytes"));
					}
					++line_offset;
					t.int_value = (t.int_value << 4) + char_to_hex[c - '0'];
					c = source[++source_pos];
					if (0 == (char_classes[c] & CCHEX)) break;
					++t.len;
				} while (true);
				t.value_tag = LexToken::TVT_INT;
				t.token_number = do_c ? CTOK_INTEGER : TK_INTEGER_CONST;
				return true;
			}
			char *end = nullptr;
			t.double_value = strtod(&source[source_pos - 1], &end);
			if (end == nullptr) end = &source[source_pos];//shouldn't happen I think.  If it does a syntax scan could be used
			t.len = (int)(end - &source[t.name]);
			line_offset += t.len - 1;
			if (t.double_value == HUGE_VAL || t.double_value == -HUGE_VAL)
			{
				source_pos += t.len - 1;
				t.token_number = -1;
				throw(std::invalid_argument("double constant out of range"));
			}
			bool looks_like_decimal = false;
			for (char *s = &source[t.name]; s < end; ++s) {
				if (0==(char_classes[*s] & CCDIGIT)) {
					source_pos += t.len - 1;
					t.value_tag = LexToken::TVT_DOUBLE;
					t.token_number = do_c ? CTOK_REAL : TK_REAL_CONST;
					return true;
				}
			}
			end = nullptr;
			t.int_value = strtoull(&source[source_pos - 1], &end, 10);
			if (end == nullptr) end = &source[source_pos];//shouldn't happen I think.  If it does a syntax scan could be used
			t.len = (int)(end - &source[t.name]);
			source_pos += t.len - 1;
			if (t.int_value == ULLONG_MAX && errno == ERANGE) {
				t.token_number = -1;
				throw(std::invalid_argument("integer constant out of range"));
			}
			t.value_tag = LexToken::TVT_INT;
			t.token_number = do_c ? CTOK_INTEGER : TK_INTEGER_CONST;
			return true;
		}
		//strings
		if (c == '"'  || (do_c && c == '\'')) {
			bool ignore = false;
			char end_char = (char)c;
			++t.name;
			//--t.len;
			for (char *s = &source[source_pos]; *s !=0 && *s != '\n'; ++s) {
				++source_pos;
				++t.len;
				++t.line_offset;
				if (!ignore && *s == end_char) {
					--t.len;
					t.token_number = do_c ? CTOK_STRING : TK_STRING_CONST;
					t.value_tag = LexToken::TVT_STRING;
					t.clean_up_data = &process_string;
					return true;
				}
				ignore = (!ignore && *s == '\\');
			}
			if (source[source_pos] == '\n') {
				++t.line;
				t.line_offset = 0;
			}
			t.token_number = -1;
			throw(std::invalid_argument("unfinished string"));

		}
		int max_punct_len = max_token_length[(uint8_t)source[t.name]];

		for (int i = 0; i < max_punct_len; ++i) if (source[t.name + i] == 0 || 0 != (char_classes[source[t.name + i]] & CCSPACE)) { max_punct_len = i; break; }

		while (max_punct_len > 2) {
			if (find(t.token_number, t.hash, &source[t.name], max_punct_len)) {
				t.len = max_punct_len;
				t.line_offset += max_punct_len - 1;
				source_pos += max_punct_len - 1;
				return true;
			}
			--max_punct_len;
		}
		if (max_punct_len >= 2) {
			int dindex;
			if (calc_double_char_index(dindex, &source[t.name]) && double_char_tokens[dindex] != -1) {
				t.token_number = double_char_tokens[dindex];
				t.len = 2;
				++source_pos;
				++t.line_offset;
				return true;
			}
		}
		if (max_punct_len >= 1)
		{
			if (single_char_tokens[c] != -1) {
				t.token_number = single_char_tokens[c];
				t.len = 1;
				return true;
			}
		}
		std::string err_str = "unrecognized character \"";
		err_str += source[t.name];
		err_str += "\"";
		t.token_number = -1;
		t.len = 1;
		dec_token();
		throw(std::invalid_argument(err_str));
	} while (true);
}
