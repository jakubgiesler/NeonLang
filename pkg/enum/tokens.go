package enum

import "fmt"

type TokenType int32

const (
	EOF TokenType = iota
	UNKNOWN
	INVALID
	WHITESPACE
	TAB
	NEWLINE

	VAR
	FN

	LPAREN
	RPAREN

	LBRACE
	RBRACE

	SEMICOLON
	DOT
	COMMA
	COLON
	EQUALS

	IDENT
	NUM
)

func TokenTypeToString(tokenType TokenType) string {
	switch tokenType {
	case EOF:
		return "EOF"
	case UNKNOWN:
		return "UNKNOWN"
	case INVALID:
		return "INVALID"
	case WHITESPACE:
		return "WHITESPACE"
	case TAB:
		return "TAB"
	case NEWLINE:
		return "NEWLINE"
	case VAR:
		return "VAR"
	case FN:
		return "FN"
	case LPAREN:
		return "LPAREN"
	case RPAREN:
		return "RPAREN"
	case LBRACE:
		return "LBRACE"
	case RBRACE:
		return "RBRACE"
	case SEMICOLON:
		return "SEMICOLON"
	case DOT:
		return "DOT"
	case COMMA:
		return "COMMA"
	case COLON:
		return "COLON"
	case EQUALS:
		return "EQUALS"
	case IDENT:
		return "IDENT"
	case NUM:
		return "NUM"
	default:
		return fmt.Sprintf("UNKNOWN TOKEN: %d", tokenType)
	}
}