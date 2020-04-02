#pragma once

namespace ether {
	struct buf_hdr {
		size_t len;
		size_t cap;
		char buf[];
	};

	#define buf__hdr(b) ((buf_hdr*)((char*)(b) - offsetof(buf_hdr, buf)))
	
	#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
	#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
	#define buf_end(b) ((b) + buf_len(b))
	#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)
	
	#define buf_free(b)        ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
	#define buf_fit(b, n)      ((n) <= buf_cap(b) ? 0 :						\
								((b) = buf__grow((b), (n), sizeof(*(b)))))
	#define buf_push(b, ...)   (buf_fit((b), 1 + buf_len(b)),				\
								(b)[buf__hdr(b)->len++] = (__VA_ARGS__))
	#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
	#define buf_clear(b)       ((b) ? buf__hdr(b)->len = 0 : 0)

	void*
	buf__grow(const void* buf, size_t new_len, size_t elem_size) {
		assert(buf_cap(buf) <= (__SIZE_MAX__ - 1) / 2);
		size_t new_cap = CLAMP_MIN(2 * buf_cap(buf), MAX(new_len, 16));
		assert(new_len <= new_cap);
		assert(new_cap <= (__SIZE_MAX__ - offsetof(buf_hdr, buf)) / elem_size);
		size_t new_size = offsetof(buf_hdr, buf) + (new_cap * elem_size);
		buf_hdr* new_hdr;

		if (buf) {
			new_hdr = (buf_hdr*)realloc(buf__hdr(buf), new_size);
		}
		else {
			new_hdr = (buf_hdr*)malloc(new_size);
			new_hdr->len = 0;
		}

		new_hdr->cap = new_cap;
		return new_hdr->buf;
	}

	template <typename T>
	struct arr {
		size_t len;
		size_t cap;
		T* mem;

		arr(void) {
			this->len = 0;
			this->cap = 0;
			this->mem = NULL;
		}
		
		void push(T elem) {
			fit();
			mem[len++] = elem;
		}

		void free(void) {
			if (mem) {
				free(mem);
			}
		
			this->len = 0;
			this->cap = 0;
		}

		T operator[] (size_t i) {
			assert(i < this->len);

			return mem[i];
		}
		
	private:
		void fit(void) {
			if (this->cap < (this->len + 1)) {
				grow(this->len + 1);
			}
		}

		void grow(size_t new_len) {
			assert(this->cap <= (__SIZE_MAX__ - 1) / 2);
			size_t new_cap = CLAMP_MIN(2 * this->cap, MAX(new_len, 16));
			assert(new_len <= new_cap);
			size_t new_size = (new_cap * sizeof(T));

			if (mem) {
				mem = (T*)realloc(mem, new_size);
			}
			else {
				mem = (T*)malloc(new_size);
			}

			this->cap = new_cap;		
		}
	};
}
