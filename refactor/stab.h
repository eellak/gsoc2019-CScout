/* 
 * (C) Copyright 2001 Diomidis Spinellis.
 *
 * The C symbol table
 *
 * $Id: stab.h,v 1.3 2001/09/14 10:09:51 dds Exp $
 */

#ifndef STAB_
#define STAB_

class Type;

// An C identifier as stored in the symbol table
class Id {
private:
	Token token;			// Any token representing the identifier
					// Needed for unification
	Type type;			// For type resolution
public:
	Id(const Token& tok, Type typ) :
		token(tok), type(typ) {}
	Id() : type(basic(b_undeclared)) {}			// Needed for map
	Type get_type() const { return type; }
	const Token& get_token() const { return token; }
	const string& get_name() const { return token.get_name(); }
};

// A symbol table instance (used (two per block) for objects and tags)
class Stab {
private:
	map<string,Id> m;
public:
	Id const* lookup(const string& s) const;
	void define(const Token& tok, const Type& typ);
	void clear() { m.clear(); }
};

class Block;

typedef vector<Block> vectorBlock;

// Encapsulate symbols with function scope
// Per ANSI these are only the labels
class Function {
private:
	static Stab label;
public:
	// Called when entering or exiting a function
	// Either call will be enough
	static void enter() { label.clear(); }
	static void exit() { label.clear(); }
	friend void label_define(const Token& tok);
};

class Block {
private:
	static int current_block;	// Current block: >= 1
	static vectorBlock scope_block;
	Stab obj;
	Stab tag;

	static void define(Stab Block::*table, const Token& tok, const Type& t);
	static Id const * lookup(const Stab Block::*table, const string& name);
public:
	static const int lu_block;	// Linkage unit definitions: 0
	static const int cu_block;	// Compilation unit definitions: 1
	static void enter();
	static void exit();
	static int get_cur_block() { return current_block; }

	// Lookup and defined
	inline friend Id const * obj_lookup(const string& name);
	inline friend void obj_define(const Token& tok, const Type& t);
};

inline void
obj_define(const Token& tok, const Type& typ)
{
	Block::define(&(Block::obj), tok, typ);
}

inline Id const *
obj_lookup(const string& name)
{
	return Block::lookup(&(Block::obj), name);
}

#endif // STAB_
