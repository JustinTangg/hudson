#include <fstream>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>
#include <utility>

using namespace std;
using namespace std::placeholders;

vector<string> tokenize (const string& row, const string& separators, bool keep_separators = false) {

  vector<string> tokenized_row;

  string::const_iterator token_begin = row.begin();
  while(token_begin != row.end()) {

    string::const_iterator token_end = 
      find_if(token_begin, row.end(),
	      [&separators](char c){ return separators.find(c) != string::npos; });

    string token(token_begin, token_end);
    tokenized_row.push_back(token);

    if(token_end == row.end()) {
      break;
    } else if(keep_separators) {
      tokenized_row.push_back(string(1, *token_end));
    }
    token_begin = ++token_end;

  }

  tokenized_row.shrink_to_fit();
  return tokenized_row;

}

vector<string> tokenize (const string& row, char separator) {
  string separators(1, separator);
  return tokenize(row, separators);
}

struct arg_options {

  vector<string> sub_args;
  
};

struct column_expression {
  vector<string> operands;
  char op;
};

struct aggregate_expression {
  string op;
  int index;
};

column_expression parse_column_expression(const string& raw_expr, const set<char>& operators) {
  
  vector<string> tokens = tokenize(raw_expr,
				   string(operators.begin(), operators.end()),
				   true);
  column_expression expr;
  if(tokens.size() != 3) {
    return expr;
  }

  expr.operands.push_back(tokens[0]);
  expr.op = tokens[1][0];
  expr.operands.push_back(tokens[2]);
  return expr;
  
}

int get_operand_value(const string& operand_str, const vector<string>& row) {
  if(operand_str.length() >= 4 and
     operand_str.substr(0, 3) == "col") {
    int col_index = stoi(operand_str.substr(3, operand_str.length()));
    if(col_index < row.size()) {
      return stoi(row[col_index]);
    } else {
      throw exception();
    }
  } else {
    return stoi(operand_str);
  }
}

int apply_operator(int l, int r, char op) {

	switch(op) {
	case '+':
	  return l + r;
	case '-':
	  return l - r;
	case '*':
	  return l * r;
	case '/':
	  return l / r;
	default:
	  throw exception();
	}

}

int average(const vector<int>& v) {
  return accumulate(v.begin(), v.end(), 0) / v.size();
}

int minimum(const vector<int>& v) {
  return *min_element(v.begin(), v.end());
}

int maximum(const vector<int>& v) {
  return *max_element(v.begin(), v.end());
}

int median(vector<int>& v) {
  sort(v.begin(), v.end());
  return v[v.size()/2];
}

void print_row(const vector<string>& row, const set<int>& included_columns, const vector<int>& max_widths) {
  for(int i = 0; i < row.size(); ++i) {
    
    if(included_columns.empty() or included_columns.count(i) > 0) {
      string format_str = "|%" + to_string(max_widths[i]+1) + "s";
      printf(format_str.c_str(), row[i].c_str());
    }
    
  }
  printf(" |\n");
}

void inner_join(const vector<string>& row,
		const vector<vector<string> >& join_rows,
		int join_column_index,
		const multimap<string, int>& join_columns,
		back_insert_iterator<vector<vector<string> > > out) {
  
  const auto& eq_range = join_columns.equal_range(row[join_column_index]);
  transform(eq_range.first,
	    eq_range.second,
	    out,
	    [&join_rows, &row](const pair<string, int>& p) {
	      vector<string> joined_row;
	      move(join_rows[p.second].begin(),
		   join_rows[p.second].end(),
		   back_inserter(joined_row));
	      move(row.begin(), row.end(), back_inserter(joined_row));
	      return move(joined_row);
	    });
}

void outer_join(const vector<string>& row,
		const vector<vector<string> >& join_rows,
		int join_column_index,
		const multimap<string, int>& join_columns,
		insert_iterator<set<int> > joined_rows_out,
		back_insert_iterator<vector<vector<string> > > out) {
  set<int> joined_rows;
  if(join_columns.count(row[join_column_index]) == 0) {
    *out++ = move(row);
    return;
  }
  auto eq_range = join_columns.equal_range(row[join_column_index]);
  transform(eq_range.first,
	    eq_range.second,
	    out,
	    [&join_rows, &row, &joined_rows](const pair<string, int>& p) {
	      vector<string> joined_row;
	      move(row.begin(), row.end(), back_inserter(joined_row));
	      move(join_rows[p.second].begin(),
		   join_rows[p.second].end(),
		   back_inserter(joined_row));
	      joined_rows.insert(p.second);
	      return move(joined_row);
	    });
  move(joined_rows.begin(), joined_rows.end(), joined_rows_out);
}

int main(int argc, char** argv) {

  char separator = ',';
  set<int> included_columns;
  set<char> operators{'+', '-', '*', '/'};
  map<string, function<int(vector<int>&)> > agg_operators
  {make_pair("avg", &average),
      make_pair("min", &minimum),
      make_pair("max", &maximum),
      make_pair("med", &median)};

  map<string, arg_options> arg_types;
  for(int i = 0; i < argc; ++i) {
    if(argv[i][0] == '-') {

      string arg_type(argv[i]);
      arg_types[arg_type] = arg_options();

      arg_options& opts = arg_types[arg_type];
      while(i+1 < argc and argv[i+1][0] != '-') {
	opts.sub_args.push_back(string(argv[++i]));
      }

    }
  }

  if(arg_types.count("-h")) {
    cout << "usage: cat file.csv | " << argv[0] << " <options>" << endl
	 << " where <options> may include any/all of: " << endl
	 << "       -cols n_1 n_2 n_3 n_4 (where n_{1,2,3,4} are valid column indices)" << endl
	 << "       -addcol col_expr where col_expr is an expression consisting of:" << endl
	 << "                             -colN where N is the column index" << endl
	 << "                             -N where N is some integer" << endl 
	 << "                             -OP where OP is a binary operator from {+, *, -, /}" << endl
	 << "       -aggcol N {avg,med,min,max} where N is a valid column index " << endl
	 << "                             avg - get average over column N" << endl
	 << "                             med - get the median over column N" << endl
	 << "                             min - get the min over column N" << endl
	 << "                             max - get the max over column N" << endl
	 << "       -innerjoin, -outerjoin N file.csv where N is a column index and file.csv is a csv file" << endl
	 << "                             (inner or outer) join the rows from stdin with the rows in file.csv." << endl
	 << "                             Joined rows' columns can be referenced in -aggcol,-addcol,and -cols expressions." << endl;
    return 0;
  }

  if(arg_types.count("-cols")) {
    for(const string& col : arg_types["-cols"].sub_args) {
      int idx = stoi(col);
      included_columns.insert(idx);
    }
  }

  column_expression expr;
  if(arg_types.count("-addcol")) {
    expr = parse_column_expression(arg_types["-addcol"].sub_args[0], operators);
  }

  vector<int> accum_vec;
  aggregate_expression agg_expr;
  if(arg_types.count("-aggcol")) {
    agg_expr = {arg_types["-aggcol"].sub_args[0],
		stoi(arg_types["-aggcol"].sub_args[1])};
  }

  vector<int> join_max_widths;
  vector<vector<string> > join_rows;
  multimap<string, int> join_columns;
  int join_column_index = -1;
  string join_type;
  if(arg_types.count("-outerjoin") > 0 or arg_types.count("-innerjoin") > 0) {

    join_type = arg_types.count("-outerjoin") ? "-outerjoin" : "-innerjoin";
    join_column_index = stoi(arg_types[join_type].sub_args[0]);
    ifstream fin(arg_types[join_type].sub_args[1].c_str());

    string current_line;
    int count = 0;
    while(getline(fin, current_line)) {

      vector<string> tokenized_row(tokenize(current_line, separator));

      if(join_column_index < tokenized_row.size()) {
	join_columns.insert(make_pair(tokenized_row[join_column_index], count));
      }

      for(int i = 0; i < tokenized_row.size(); ++i) {

	int width = tokenized_row[i].size();

	if(i >= join_max_widths.size()) {
	  join_max_widths.push_back(width);
	} else if(width > join_max_widths[i]) {
	  join_max_widths[i] = width;
	}

      }

      join_rows.push_back(tokenized_row);
      ++count;

    }

    fin.close();

  }

  string current_line;
  vector<vector<string> > rows;
  vector<int> max_widths;

  while(getline(cin, current_line)) {

    vector<string> tokenized_row(tokenize(current_line, separator));

    if(max_widths.empty()) {

      for(const string& col : tokenized_row) {
	max_widths.push_back(col.size());
      }

    } else {

      for(int i = 0; i < tokenized_row.size(); ++i) {

	int width = tokenized_row[i].size();

	if(i >= max_widths.size()) {
	  max_widths.push_back(width);
	} else if(width > max_widths[i]) {
	  max_widths[i] = width;
	}

      }

    }

    if(expr.operands.size() == 2 and operators.count(expr.op) > 0) {
      
      try {

	int l_operand = get_operand_value(expr.operands[0], tokenized_row);
	int r_operand = get_operand_value(expr.operands[1], tokenized_row);
	string result = to_string(apply_operator(l_operand, r_operand, expr.op));
	tokenized_row.push_back(result);

	int width = result.size();

	if(tokenized_row.size() > max_widths.size()) {
	  max_widths.push_back(width);
	} else if(width > *max_widths.rbegin()) {
	  *max_widths.rbegin() = width;
	}

      } catch (const exception& e) {
	cerr << "warning: invalid column expression: '" << e.what() << "', skipping..." << endl;
      }
    }

    if(agg_operators.count(agg_expr.op) > 0 and agg_expr.index < tokenized_row.size()) {
      accum_vec.push_back(stoi(tokenized_row[agg_expr.index]));
    }

    rows.push_back(tokenized_row);

  }

  rows.shrink_to_fit();

  vector<int> extended_max_widths(max_widths);
  vector<vector<string> > join_result;

  if(not join_rows.empty()) {
    move(join_max_widths.begin(), join_max_widths.end(), back_inserter(extended_max_widths));
    if(join_type == "-outerjoin") {
      set<int> joined_rows;
      for_each(rows.begin(), rows.end(),
	       bind(&outer_join,
		    _1,
		    join_rows,
		    join_column_index,
		    join_columns,
		    inserter(joined_rows, joined_rows.begin()),
		    back_inserter(join_result)));
      for(int i = 0; i < join_rows.size(); ++i) {
	if(joined_rows.count(i) == 0) {
	  vector<string> left_null_row(max_widths.size());
	  move(join_rows[i].begin(), join_rows[i].end(), back_inserter(left_null_row));
	  join_result.push_back(left_null_row);
	}
      }
    } else if(join_type == "-innerjoin") {
      for_each(rows.begin(), rows.end(),
	       bind(&inner_join,
		    _1,
		    join_rows,
		    join_column_index,
		    join_columns,
		    back_inserter(join_result)));
    } else {
      cerr << "no join type specified!" << endl;
      return 1;
    }
    rows.clear();
    join_rows.clear();
  }

  join_result.shrink_to_fit();

  if(join_result.empty()) {
    for(vector<string>& row: rows) {
      if(expr.operands.size() == 2 and operators.count(expr.op) > 0) {
	
	try {
	  
	  int l_operand = get_operand_value(expr.operands[0], row);
	  int r_operand = get_operand_value(expr.operands[1], row);
	  string result = to_string(apply_operator(l_operand, r_operand, expr.op));
	  row.push_back(result);
	  
	  int width = result.size();
	  
	  if(row.size() > max_widths.size()) {
	    max_widths.push_back(width);
	  } else if(width > *max_widths.rbegin()) {
	    *max_widths.rbegin() = width;
	  }
	  
	} catch (const exception& e) {
	  cerr << "warning: invalid column expression: '" << e.what() << "', skipping..." << endl;
	}
      }
      
      if(agg_operators.count(agg_expr.op) > 0 and agg_expr.index < row.size()) {
	try {
	  accum_vec.push_back(stoi(row[agg_expr.index]));
	} catch(const exception& e) {
	  cerr << "aggregate column value not integer for row." << endl;
	}
      }
      
    }
    for(const vector<string>& row: rows) {
      print_row(row, included_columns, max_widths);
    }
  } else {
    for(vector<string>& row: join_result) {
      if(expr.operands.size() == 2 and operators.count(expr.op) > 0) {
	
	try {
	  
	  int l_operand = get_operand_value(expr.operands[0], row);
	  int r_operand = get_operand_value(expr.operands[1], row);
	  string result = to_string(apply_operator(l_operand, r_operand, expr.op));
	  row.push_back(result);
	  
	  int width = result.size();
	  
	  if(row.size() > extended_max_widths.size()) {
	    extended_max_widths.push_back(width);
	  } else if(width > *extended_max_widths.rbegin()) {
	    *extended_max_widths.rbegin() = width;
	  }
	  
	} catch (const exception& e) {
	  cerr << "warning: invalid column expression: '" << e.what() << "', skipping..." << endl;
	}
      }
      
      if(agg_operators.count(agg_expr.op) > 0 and agg_expr.index < row.size()) {
	try {
	  accum_vec.push_back(stoi(row[agg_expr.index]));
	} catch(const exception& e) {
	  cerr << "aggregate column value not integer for row." << endl;
	}
      }
      
    }
    for(const vector<string>& row: join_result) {
      print_row(row, included_columns, extended_max_widths);
    }
  }

  if(agg_operators.count(agg_expr.op) > 0) {
    cout << agg_expr.op << ": " << agg_operators.at(agg_expr.op)(accum_vec) << endl;
  }

  return 0;

}
