	namespace interleaved {
		/** Interleaved frames: L,R,L,R for two channels. Sizes and offsets are frames.
		 * A float-pointer constructor borrows host storage; the size constructor owns
		 * storage rounded up to a power of two. Copies borrow the same storage and
		 * rewind; the original owner must outlive them. Assignment copies audio.
		 * Owned integer indexing wraps at allocated frame capacity, like mono buffer.
		 * Example: interleaved::buffer<2> audio(hostSamples, frameCount);
		 */
		template<int CHANNELS = 2>
		class buffer {
		public:
			using signal = signals<CHANNELS>;
			static_assert(CHANNELS > 0);
			static_assert(std::is_trivially_copyable_v<signal>);
			static_assert(std::is_standard_layout_v<signal>);
			static_assert(sizeof(signal) == sizeof(float) * CHANNELS);
			static_assert(alignof(signal) == alignof(float));
		private:
			static unsigned int capacity(unsigned int size) {
				if (size == 0) return 1;
				--size;
				size |= size >> 1;
				size |= size >> 2;
				size |= size >> 4;
				size |= size >> 8;
				size |= size >> 16;
				return size + 1;
			}
			const unsigned int mask = 0xFFFFFFFF;
			const bool owned = false;
			float* samples;
			signal* ptr;
			signal* end;
		public:
			const int size;
			buffer(float* samples, int size) : samples(samples), size(size) {
				assert(size >= 0 && (samples || size == 0));
				rewind();
			}
			buffer(float* samples, int size, float initial) : buffer(samples, size) {
				set(initial);
			}
			buffer(int size = 1, float initial = 0)
				: mask(capacity(size) - 1), owned(true),
				  samples(reinterpret_cast<float*>(new signal[capacity(size)])), size(size) {
				assert(size >= 0);
				rewind();
				set(initial);
			}
			buffer(const buffer& other)
				: mask(other.mask), samples(other.samples), size(other.size) { rewind(); }
			~buffer() {
				if (owned) delete[] reinterpret_cast<signal*>(samples);
			}
			void rewind(int offset = 0) {
				assert(offset >= 0 && offset <= size);
				ptr = samples ? reinterpret_cast<signal*>(samples) + offset : nullptr;
				end = samples ? reinterpret_cast<signal*>(samples) + size : nullptr;
			}
			int offset() const { return samples ? int(ptr - reinterpret_cast<signal*>(samples)) : 0; }
			void clear() { clear(size); }
			void clear(int count) {
				assert(count >= 0);
				if (count > 0 && size > 0) std::memset(samples, 0, sizeof(signal) * std::min(count, size));
			}
			void set(float value = 0) {
				if (value == 0) clear();
				else for (size_t i = 0; i < size_t(size) * CHANNELS; ++i) samples[i] = value;
			}
			signal& operator[](int index) {
				return reinterpret_cast<signal*>(samples)[static_cast<unsigned int>(index) & mask];
			}
			const signal& operator[](int index) const {
				return reinterpret_cast<const signal*>(samples)[static_cast<unsigned int>(index) & mask];
			}
			signal operator[](float offset) const {
				const float whole = std::floor(offset);
				const float fraction = offset - whole;
				const int i = int(whole), j = i == size - 1 ? 0 : i + 1;
				return (*this)[i] * (1.f - fraction) + (*this)[j] * fraction;
			}
			signal operator[](float offset) { return static_cast<const buffer&>(*this)[offset]; }
			operator signal& () { return *ptr; }
			operator const signal& () const { return *ptr; }
			bool finished() const { return ptr == end; }
			signal& operator++(int) { return *ptr++; }
			signal& operator=(const signal& value) { return *ptr = value; }
			signal& operator+=(const signal& value) { return *ptr += value; }
			signal& operator*=(const signal& value) { return *ptr *= value; }
			buffer& operator=(const buffer& other) {
				if (size > 0 && other.size > 0)
					std::memmove(samples, other.samples, std::min(size, other.size) * sizeof(signal));
				return *this;
			}
			buffer& operator<<(const signal& value) { *ptr = value; return *this; }
			float* data() { return samples; }
			const float* data() const { return samples; }
		};
	}

