#include "log_file.h"

char log_file::delimiter = '\n';

log_file::log_file(string &name) {
	string file_name = name + ".log";
	int position = -1;
	check_file(file_name, position);
	assert(position != -1);
	pthread_mutex_init(&cache_mutex, NULL);
	pthread_mutex_init(&writer_mutex, NULL);
	pthread_mutex_init(&reader_mutex, NULL);
	pthread_cond_init(&cache_cv, NULL);
	//turn off ios::trunc
	writer.open(file_name.c_str(), ios::in|ios::out);
	reader.open(file_name.c_str(), ios::in|ios::out);
	assert(writer.is_open());
	assert(reader.is_open());
	writer.seekp(position);
	first_cached = w_next;
	r_next = 0;
}

log_file::~log_file() {
	kvs_error("@~log_file: log_file's destructor shouldn't be called!\n");
}

bool
log_file::read(int number, string &record, int timeout) {
	//there may be a gap in cache.
	pthread_mutex_lock(&reader_mutex);
	if (first_cached > r_next) {
		if (number > r_next) {
			pthread_mutex_unlock(&reader_mutex);
			return false;
		}
		if (number < r_next) {
			pthread_mutex_lock(&cache_mutex);
			record = cache[number];
			pthread_mutex_unlock(&cache_mutex);
			pthread_mutex_unlock(&reader_mutex);
			return true;
		}
		string line;
		getline(reader, line);
		int num;
		parse_line(line, num, record);
		assert(num == number);
		//install entry.
		pthread_mutex_lock(&cache_mutex);
		cache[num] = record;
		pthread_mutex_unlock(&cache_mutex);
		r_next++;
		pthread_mutex_unlock(&reader_mutex);
		return true;
	}
	pthread_mutex_unlock(&reader_mutex);
	pthread_mutex_lock(&cache_mutex);
	if (timeout == 0) {
		if (cache.find(number) == cache.end()) {
			pthread_mutex_unlock(&cache_mutex);
			return false;
		} else {
			record = cache[number];
			pthread_mutex_unlock(&cache_mutex);
			return true;
		}
	}
	struct timespec ts;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts.tv_sec  = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;
	ts.tv_sec += timeout;
	while (cache.find(number) == cache.end()) {
		if (pthread_cond_timedwait(&cache_cv, &cache_mutex, &ts) == ETIMEDOUT) {
			pthread_mutex_unlock(&cache_mutex);
			return false;
		}
	}
	record = cache[number];
	pthread_mutex_unlock(&cache_mutex);
	return true;
}

int
log_file::size() {
	return w_next;
}

void
log_file::write(int number, string &record) {
	//a simple infinite-memory write-through cache.
	pthread_mutex_lock(&cache_mutex);
	//make sure the caller has enforced sequential writing.
	assert(w_next == number);
	w_next++;
	cache[number] = record;
	pthread_cond_signal(&cache_cv);
	pthread_mutex_lock(&writer_mutex);
	//let data expose to reader ASAP.
	pthread_mutex_unlock(&cache_mutex);
	writer << number;
	writer << ' ';
	writer << record;
	writer << '\n';
	writer.flush();
	assert(writer.good());
	pthread_mutex_unlock(&writer_mutex);
}

void
log_file::check_file(string &file_name, int &position) {
	fstream fs(file_name.c_str(), ios::in);
	assert(fs.is_open());
	fs.seekg(0, ios::end);
	if (fs.tellg() == 0) {
		w_next = 0;
		position = 0;
		fs.close();
		return;
	}
	int i = -1;
	while (1) {
		fs.seekg(i, ios::end);
		char c = fs.peek();
		if (c == delimiter) {
			int pos_n = fs.tellg();
			position = pos_n+1;
			break;
		}
		if (fs.tellg() == 0) {
			position = 0;
			w_next = 0;
			fs.close();
			return;
		}
		i--;
	}
	while (1) {
		if (fs.tellg() == 0) {
			w_next = 0;
			fs.close();
			return;
		}
		i--;
		fs.seekg(i, ios::end);
		char c = fs.peek();
		if (c == delimiter) {
			string line;
			c = 0;
			fs.read(&c, 1);
			assert(c == delimiter);
			//now delimiter is '\n', so no explicit argument.
			getline(fs, line);
			fs.close();
			int number;
			string value;
			parse_line(line, number, value);
			w_next = number + 1;
			break;
		}
	}
}

void
log_file::parse_line(string &line, int &number, string &value) {
	stringstream buffer;
	buffer << line;
	buffer >> number;
	buffer >> value;
}



