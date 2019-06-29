/*
 * (C) Copyright 2007-2015 Diomidis Spinellis
 *
 * You may only use this code if you agree to the terms of the CScout
 * Source Code License agreement (see License.txt).
 * If you do not agree to the terms, do not use the code.
 *
 * Encapsulates a (user interface) metrics query part
 * Can be used to evaluate elements referenced through E against M metrics
 *
 */

#ifndef MQUERY_
#define MQUERY_

#include <vector>
#include <sstream>

using namespace std;

#include "query.h"
#include "headers.h"
#include "metrics.h"

template <class M, class E>
class MQuery {
	// Query arguments
	char match_type;	// Type of boolean match
	int sort_order;		// Order to use for sorting
	bool reverse;		// Reverse the sort order
	vector <int> op;
	vector <int> n;
public:
	MQuery() :
		sort_order(-1),
		reverse(0),
		op(M::metric_max, 0),
		n(M::metric_max, 0)
	{
		for (int i = 0; i < M::metric_max; i++) {
			ostringstream argspec;

			argspec << "|i(c" << i << ")";
			argspec << "i(n" << i << ")";
			op[i] = server.getIntParam("c"+to_string(i));
			n[i] = server.getIntParam("n"+to_string(i));
		}
		if (!(sort_order = server.getIntParam("order")))
			sort_order = -1;
		reverse = !!server.getIntParam("reverse");
	}

	// Accessor methods
	void set_match_type(char mt) { match_type = mt; }
	int get_sort_order() const { return (sort_order); }
	bool get_reverse() const { return (reverse); }

	// Return the URL for re-executing this query part
	string param_url() const {
		ostringstream url;

		for (int i = 0; i < M::metric_max; i++)
			if (op[i]) {
				url << "&c" << i << '=' << op[i];
				url << "&n" << i << '=' << n[i];
			}
		if (sort_order != -1)
			url << "&order=" << sort_order;
		if (reverse)
			url << "&reverse=1";
		return url.str();
	}

	// Evaluate the stored query against e
	bool eval(E e) {
		bool add;

		switch (match_type) {
		default:
		case 'Y':	// anY match
			add = false;
			for (int j = 0; j < M::metric_max; j++)
				if (op[j] && Query::apply(op[j], e.metrics().get_metric(j), n[j])) {
					add = true;
					break;
				}
			break;
		case 'L':	// alL match
		case 'T':	// exactT match
			add = true;
			for (int j = 0; j < M::metric_max; j++)
				if (op[j] && !Query::apply(op[j], e.metrics().get_metric(j), n[j])) {
					add = false;
					break;
				}
			break;
		case 'E':	// excludE match
			add = true;
			for (int j = 0; j < M::metric_max; j++)
				if (op[j] && Query::apply(op[j], e.metrics().get_metric(j), n[j])) {
					add = false;
					break;
				}
			break;
		}
		return (add);
	}

	// Generate and return form's metrics query part
	// {
	// 		header: "html table head",
	// 		table: [
	// 			"html of table row "
	// 		],
	// 		end: "html table end",
	//		metrics: [
	//			"metrics names"	
	//		]
	// }
	static json::value metrics_query_form() {
		json::value to_return;
	
		to_return["header"] = json::value::string("<table>"
			"<tr><th>Sort-by</th><th>Metric</th><th>Compare</th><th>Value</th></tr>\n");
		for (int i = 0; i < M::metric_max; i++) {
			cout<<i<<endl;
			if (Metrics::is_internal<M>(i))
				continue;
			to_return["metrics"][i] = json::value(Metrics::get_name<M>(i));
			to_return["table"][i] = json::value::string("<tr><td><input type=\"radio\" name=\"order\" value=\""
			+ to_string(i) + "\"> </td>\n"
			"<td>" + Metrics::get_name<M>(i) + "</td><td><select name=\"c" +
			to_string(i) + "\" value=\"1\">\n" +
			Query::equality_selection() +
			"</td><td><INPUT TYPE=\"text\" NAME=\"n"+to_string(i) + "\" SIZE=5 MAXLENGTH=10></td></tr>\n");
		}
		
		to_return["end"] = json::value::string("<tr>"
			"<td><input type=\"radio\" name=\"order\" value=\"-1\" CHECKED></td>\n"
			"<td>Entity name</td>"
			"<td></td><td></td></tr>"
			"</table>\n"
			"<p>"
			"<input type=\"checkbox\" name=\"reverse\" value=\"0\">Reverse sort order\n");
		return to_return;
	}
};

#endif /* MQUERY_ */
