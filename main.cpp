#include <fstream>
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
      cout << '|' << setw(max_widths[i]+1) << row[i] << " ";
    }
    
  }
  cout << '|' << endl;
}

void print_join_row(const pair<string, int>& item,
		    const vector<string>& row,
		    const vector<vector<string> >& join_rows,
		    const set<int>& included_columns,
		    const vector<int>& max_widths) {
  vector<string> final_row(row);
  copy(join_rows[item.second].begin(), join_rows[item.second].end(), back_inserter(final_row));
  print_row(final_row, included_columns, max_widths);
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

      } catch (const exception e) {
	cerr << "warning: invalid column expression, skipping..." << endl;
      }
    }

    if(agg_operators.count(agg_expr.op) > 0 and agg_expr.index < tokenized_row.size()) {
      accum_vec.push_back(stoi(tokenized_row[agg_expr.index]));
    }

    rows.push_back(tokenized_row);

  }

  set<int> joined_rows;
  vector<int> extended_max_widths(max_widths);
  copy(join_max_widths.begin(), join_max_widths.end(), back_inserter(extended_max_widths));
  for(const vector<string>& row: rows) {

    int join_count = join_columns.count(row[join_column_index]);

    if(join_column_index < 0 or (join_count == 0 and join_type == "-outerjoin")) {

      print_row(row, included_columns, max_widths);

    }

    if(join_column_index >= 0 and join_count > 0) {

      auto eq_range = join_columns.equal_range(row[join_column_index]);
      for_each(eq_range.first, eq_range.second, bind(&print_join_row, _1, row, join_rows, included_columns, extended_max_widths));
      transform(eq_range.first,
		eq_range.second,
		inserter(joined_rows, joined_rows.begin()),
		[](const pair<string, int>& p) { return p.second; });

    }

  }

  if(join_column_index >= 0 and join_type == "-outerjoin") {
    for(int i = 0; i < join_rows.size(); ++i) {
      if(joined_rows.count(i) == 0) {
	vector<string> extended_row(max_widths.size());
	copy(join_rows[i].begin(), join_rows[i].end(), back_inserter(extended_row));
	print_row(extended_row, included_columns, extended_max_widths);
      }
    }
  }

  if(agg_operators.count(agg_expr.op) > 0) {
    cout << agg_expr.op << ": " << agg_operators.at(agg_expr.op)(accum_vec) << endl;
  }

  return 0;

}
