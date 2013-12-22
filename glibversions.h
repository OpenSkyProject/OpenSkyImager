#if GLIB_MINOR_VERSION < 32
	#define g_rw_lock_init(lock)                      g_static_rw_lock_init(lock)
	#define g_rw_lock_clear(lock)                     g_static_rw_lock_clear(lock)
	#define g_rw_lock_writer_lock(lock)               g_static_rw_lock_writer_lock(lock)
	#define g_rw_lock_writer_trylock(lock)            g_static_rw_lock_writer_trylock(lock)
	#define g_rw_lock_writer_unlock(lock)             g_static_rw_lock_writer_unlock(lock)
	#define g_rw_lock_reader_lock(lock)               g_static_rw_lock_reader_lock(lock)
	#define g_rw_lock_reader_trylock(lock)            g_static_rw_lock_reader_trylock(lock)
	#define g_rw_lock_reader_unlock(lock)             g_static_rw_lock_reader_unlock(lock)
	#define g_thread_try_new(name, func, data, error) g_thread_create(func, data, TRUE, error)
	#define g_thread_new(name, func, data)            g_thread_create(func, data, TRUE, NULL)
	#define g_thread_unref(thd)                       //g_thread_unref(thd)
#endif


