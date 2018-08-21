#include <bits/stdc++.h>
#include "lib/json.hpp"
#include "lib/utf8.h"

using json = nlohmann::json;

int hex_value(char c) {
	if ('0' <= c && c <= '9') {
		return c - '0';
	} else {
		return 10 + (c - 'a');
	}
}

int hexstr_to_int(const std::string &n) {
	int res = 0;
	for (int i = 0; i < n.size(); ++i) {
		res *= 16;
		res += hex_value(n[i]);
	}
	return res;
}

std::string decode(const std::string &s) {
	std::string res;
	bool escaped = false;

	for (int i = 0; i < s.size(); ++i) {
		if (escaped) {
			if (s[i] == 'n' || s[i] == '\"' || s[i] == '\'' || s[i] == '\\' || s[i] == '?') {
				res += "\\";
				res += s[i];
			} else if (s[i] == 'u' && i + 4 < s.size()) {
				res += char(hex_value(s[i + 3]) << 4 | hex_value(s[i + 4]));
				i += 4;
			} else {
				res += s[i];
			}
			escaped = false;
		} else {
			if (s[i] == '\\') {
				escaped = true;
				continue;
			} else {
				res += s[i];
			}
		}
	}
	
	return res;
}

json read_json_from_file(const std::string &file_name) {
	std::ifstream f(file_name);
	std::stringstream buffer;
	buffer << f.rdbuf();
	f.close();

	std::string decoded_string = decode(buffer.str());

	std::ofstream fout("decoded.json");
	fout << decoded_string << std::endl;
	fout.close();

	json res;
	std::istringstream(decoded_string) >> res;
	return res;
}

std::string epoch_to_date(int64_t epoch) {
	time_t t = epoch / 1000;
	std::string date = ctime(&t);
	return date.substr(0, (int) date.size() - 1);
}

struct html {
	std::string head;
	std::string buffer;
	std::string tail;

	html() {
		set_head();
		set_tail();
	}

	void set_head() {
		head += "<!DOCTYPE html>\n";
		head += "<html>\n";
		head += "<head>\n";
		head += "	<link rel=\"stylesheet\" href=\"styles.css\">";
		head += "</head>\n";
		head += "<body>\n";
		head += "<ul>\n";
	}

	void set_tail() {
		tail += "</ul>";
		tail += "</body>\n";
		tail += "</html>\n";
	}

	void raw_div(const std::string &content) {
		buffer += "<div>" + content + "</div>";
	}

	void chat(const std::string &content, const std::string &css_class, const std::string &date) {
		buffer += "<li title=\"" + date + "\" " + "class=\"" + css_class + "\">" + content + "</li>";
	}

	void image(const std::string &uri, const std::string &css_class) {
		buffer += "<li class=\"" + css_class + "\">" + "<img src=\"" + uri + "\">" + "</li>";
	}

	void date(const std::string &date) {
		buffer += "<li class=\"date\">" + date + "</li>";
	}

	friend std::ostream& operator << (std::ostream &out, const html &h);
};

std::ostream& operator << (std::ostream &out, const html &h) {
	out << h.head;
	out << h.buffer;
	out << h.tail;
	return out;
}

std::vector< std::string > split(const std::string &s, char separator = '\n') {
	std::vector< std::string > res;
	int last_pos = -1;
	for (int i = 0; i <= s.size(); ++i) {
		if (i == s.size() || s[i] == separator) {
			if (last_pos + 1 < i) {
				res.push_back(s.substr(last_pos + 1, i - last_pos - 1));
			}
			last_pos = i;
		}
	}
	return res;
}

void process(const std::string &prefix) {
	json parsed = read_json_from_file(prefix + "/message.json");
	std::vector< json > participants = parsed["participants"];
	std::string my_name = participants.back()["name"];
	html holder;
	std::reverse(begin(parsed["messages"]), end(parsed["messages"]));
	
	int64_t last_epoch = 0;
	std::map<std::string, int> first_count;
	std::map<std::string, int> message_count;
	std::map<std::string, int> sum_length;

	for (const json &message : parsed["messages"]) {
		bool mine = message["sender_name"] == my_name;
		//holder.div(epoch_to_date(message["timestamp_ms"]), "date");
		//holder.div(message["sender_name"], "name");
		int64_t now_epoch = message["timestamp_ms"];
		if ((now_epoch - last_epoch) / 1000 / 60 / 60 > 24) {
			first_count[message["sender_name"]]++;
			holder.date(epoch_to_date(now_epoch));
			last_epoch = now_epoch;
		}
		if (message.count("content")) {
			std::string content = message["content"];
			message_count[message["sender_name"]] += 1;
			sum_length[message["sender_name"]] += utf8::distance(content.begin(), utf8::find_invalid(content.begin(), content.end()));
			for (const std::string &line : split(message["content"])) {
				holder.chat(line, mine ? "mine" : "your", epoch_to_date(message["timestamp_ms"]));
			}
		} else if (message.count("sticker")) {
			message_count[message["sender_name"]] += 1;
			sum_length[message["sender_name"]] += 1;
			std::string uri = message["sticker"]["uri"];
			holder.image(uri, mine ? "mine-img" : "your-img");
		} else if (message.count("photos")) {
			message_count[message["sender_name"]] += 1;
			sum_length[message["sender_name"]] += 1;
			for (const json &uri_element : message["photos"]) {
				std::string uri = uri_element["uri"];
				holder.image(uri, mine ? "mine-img" : "your-img");
			}
		} else {
			message_count[message["sender_name"]] += 1;
			sum_length[message["sender_name"]] += 1;
		}
	}

	std::string everyone;
	for (const json &it : participants) {
		everyone += it["name"];
		if (&it != &participants.back()) {
			everyone += "_";
		}
	}
	std::ofstream fout("output_html/" + everyone + ".html");
	fout << holder << std::endl;
	fout.close();

	std::cerr << "# summary on " << everyone << std::endl;
	for (const json &it : participants) {
		std::string name = it["name"];
		std::cerr << name << ' ' << first_count[name] << ' ' << message_count[name] << ' ' << sum_length[name] << ' ' << sum_length[name] / message_count[name] << std::endl;
	}
	std::cerr << std::endl;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i) {
		std::string prefix(argv[i]);
		process(prefix);
	}
	return 0;
}