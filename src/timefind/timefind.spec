[__many__]
paths = force_list(default=list())
include = force_list(default=list())
exclude = force_list(default=list())
extractor = string(default=None)
file_time_is_end_time = boolean(default=False)
indexer = string(default=None)
index_path = string(default=None)
index_type = option('sqlite_nfs', 'sqlite', default='sqlite')

	[[__many__]]
		paths = force_list(default=list())
		include = force_list(default=list())
		exclude = force_list(default=list())
		extractor = string(default=None)
		file_time_is_end_time = boolean(default=False)
		indexer = string(default=None)
		index_path = string(default=None)
		index_type = option('sqlite_nfs', 'sqlite', default='sqlite')
