#ifndef FILE_SIZE_DETECTER_H__
#define FILE_SIZE_DETECTER_H__

#include <boost/filesystem.hpp>
#include<boost/function.hpp>

namespace bfs = boost::filesystem;

class file_size_detecter{
public:
	enum{
		err_file_not_found,
		err_file_illegal,
		err_dir_iterator,
		err_file_size,
	};
public:
	file_size_detecter(const boost::filesystem::path &path);
	file_size_detecter();
	//~file_size_detecter();
public:
	// 开始处理
	void start();

	void set_path(const boost::filesystem::path &path);

	// 注册处理器
	template<typename Handler> void register_file_end_handler(Handler handler);

	template<typename Handler> void register_dir_beg_handler(Handler handler);

	template<typename Handler> void register_dir_end_handler(Handler handler);

	template<typename Handler> void register_err_handler(Handler handler);
private:
	void process_dir(const boost::filesystem::path &path);
	void process_file(const boost::filesystem::path &path);

	void do_process_dir(const boost::filesystem::path &path, boost::system::error_code &err_code);

private:
	boost::filesystem::path m_path;

	boost::function<void (const boost::filesystem::path &, uintmax_t)> m_file_end_handler;
	boost::function<void (const boost::filesystem::path &)> m_dir_beg_handler;
	boost::function<void (const boost::filesystem::path &)> m_dir_end_handler;
	boost::function<void (int, const std::wstring &)>       m_err_handler;
};

inline file_size_detecter::file_size_detecter(){}
inline file_size_detecter::file_size_detecter(const boost::filesystem::path &path) : m_path(path) { }

inline void file_size_detecter::set_path(const boost::filesystem::path &path)
{
	m_path = path;
}

inline void file_size_detecter::start()
{
	if (!bfs::exists(m_path) && m_err_handler)
		m_err_handler(err_file_not_found, m_path.wstring() );
	else
	{
		if (bfs::is_directory(m_path))
			process_dir(m_path);
		else if (bfs::is_regular_file(m_path))
			process_file(m_path);
		else{
			m_err_handler(err_file_illegal, m_path.wstring());
		}
	}
}

template<typename Handler> 
inline void file_size_detecter::register_file_end_handler(Handler handler) { m_file_end_handler = handler; }

template<typename Handler> 
void file_size_detecter::register_dir_beg_handler(Handler handler){ m_dir_beg_handler = handler; }

template<typename Handler> 
inline void file_size_detecter::register_dir_end_handler(Handler handler) { m_dir_end_handler = handler; }

template<typename Handler> 
void file_size_detecter::register_err_handler(Handler handler) { m_err_handler = handler; }


inline void file_size_detecter::process_dir(const boost::filesystem::path &path)
{
	if (m_dir_beg_handler)
		m_dir_beg_handler(path);

	boost::system::error_code ec;
	do_process_dir(path, ec);

	if (ec)
	{
		if (m_err_handler)
			m_err_handler(err_dir_iterator, path.wstring());
	}

	if (m_dir_end_handler)
		m_dir_end_handler(path);
}

inline void file_size_detecter::process_file(const boost::filesystem::path &path)
{
	boost::system::error_code err;

	uintmax_t size = bfs::file_size(path, err);

	if (err)
	{
		if (m_err_handler)
			m_err_handler(err_file_size, path.wstring() );
		else
			m_file_end_handler(path, 0);
	}
	else
	{
		if (m_file_end_handler)
			m_file_end_handler(path, size);
	}
}

void file_size_detecter::do_process_dir(const boost::filesystem::path &path, boost::system::error_code &ec)
{
	bfs::directory_iterator dirit(path, ec);
	if (ec) return;

	bfs::directory_iterator dirend;
	for (; dirit != dirend; ++dirit)
	{
		if ( bfs::is_directory(dirit->status()) )
			process_dir(dirit->path());
		else
			process_file(dirit->path());
	}
}

#endif