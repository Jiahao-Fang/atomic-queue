#pragma once
// this is a library for a thread safe queue
// will use concepts of c++20   


#include <atomic>
#include <concepts>
#include <new>
#include <cassert>
#include <cstddef>


namespace sl{
    struct EnablePowerOfTwo{};
    struct DisablePowerOfTwo{};
    // two struct tag, heap and stack buffer
    struct UseHeapBuffer{};
    struct UseStackBuffer{};
    #if defined(__cpp_lib_hardware_interference_size)
    static constexpr size_t cache_line = std::hardware_destructive_interference_size;
    #else
    static constexpr size_t cache_line = 64;
    #endif
    static constexpr bool is_power_of_two(size_t value) noexcept{
        return !(value & (value - 1));
    }
    // define concepts
    template<typename T>
    concept IsEnablePowerOfTwo = std::is_same_v<T, EnablePowerOfTwo>;
    template<typename T>
    concept IsDisablePowerOfTwo = std::is_same_v<T, DisablePowerOfTwo>;
    template<typename T>
    concept IsHeapBuffer = std::is_same_v<T, UseHeapBuffer>;
    template<typename T>
    concept IsStackBuffer = std::is_same_v<T, UseStackBuffer>;
    template<typename T>
    concept IsValidConstraint = IsEnablePowerOfTwo<T> || IsDisablePowerOfTwo<T>;
    template<typename T>
    concept IsAligned = alignof(T) % cache_line == 0;
    template<typename T>
    concept SizeMultipleOfCacheLine = sizeof(T) % cache_line == 0;
    template<typename T>
    concept FalseSharingSafe = IsAligned<T> && SizeMultipleOfCacheLine<T>;
    template<size_t N, typename SizeConstraint>
    concept ValidSizeParameter = IsValidConstraint<SizeConstraint> && (IsDisablePowerOfTwo<SizeConstraint> || is_power_of_two(N));

    template<typename T>
    concept IsTrivial = sizeof(T) <= sizeof(uint64_t)
                        &&std::is_trivially_copyable_v<T>
                        &&std::is_trivially_destructible_v<T>;
    template<typename T>
    concept IsValidBufferType = IsHeapBuffer<T> || IsStackBuffer<T>;
    // to do plus1
    template<typename T,IsValidConstraint SizeConstraint,bool Modulo,size_t N=0>
    // heap buffer
    class HeapBuffer{
        private:
        T *buffer_;
        const size_t buffer_size_;
        const size_t buffer_mask_;
        std::allocator<T> allocator_;
        public:
        explicit HeapBuffer(const size_t size,const std::allocator<T>& allocator = std::allocator<T>()):
        buffer_size_(size),
        buffer_mask_(size-1),
        allocator_(allocator){
            buffer_ = allocator_.allocate(buffer_size_+1);
        }
        ~HeapBuffer() noexcept{
            allocator_.deallocate(buffer_,buffer_size_+1);
        }
        HeapBuffer(const HeapBuffer&) = delete;
        HeapBuffer& operator=(const HeapBuffer&) = delete;
        T& operator[](const size_t index) noexcept{
            if constexpr(!Modulo){
                return buffer_[index];
            }else if constexpr(IsEnablePowerOfTwo<SizeConstraint>){
                return buffer_[index&buffer_mask_];
            }else if constexpr(N!=0){
                return buffer_[index%N];
            }else{
                return buffer_[index%buffer_size_];
            }
        }
        const T& operator[](const size_t index) const noexcept{
            if constexpr(!Modulo){
                return buffer_[index];
            }else if constexpr(IsEnablePowerOfTwo<SizeConstraint>){
                return buffer_[index&buffer_mask_];
            }else if constexpr(N!=0){
                return buffer_[index%N];
            }else{
                return buffer_[index%buffer_size_];
            }
        }
    };
    template<typename T,bool Modulo,size_t N=0>
    // stack buffer. compilor will optimize the index%N to index&N-1
    class StackBuffer{
        private:
        std::array<T,N> buffer_;
        public:
        StackBuffer(auto, auto) noexcept {}
        ~StackBuffer() noexcept = default;
        T& operator[](const size_t index) noexcept{
            if constexpr(!Modulo){
                return buffer_[index];
            }else{
                return buffer_[index%N];
            }
        }
        const T& operator[](const size_t index) const noexcept{
            if constexpr(!Modulo){
                return buffer_[index];
            }else{
                return buffer_[index%N];
            }
        }
    };
    template <bool HasSeq>
    struct SeqField;
    
    // specializations for HasSeq
    template<>
    struct SeqField<true>{
        alignas(cache_line) std::atomic<size_t> seq_;
        SeqField(size_t init_value = 0) noexcept :seq_(init_value){}
    };
    template<>
    struct SeqField<false>{
        SeqField(size_t) noexcept {}
    };
    template<bool HasSeq,bool TriviallyDestructible>
    struct IsConstructedField;

    // specializations for IsConstructedField
    template<bool HasSeq>
    struct IsConstructedField<HasSeq,true>{
        IsConstructedField(bool isconstructed){}
    };
    template<>
    struct IsConstructedField<true,false>{
        bool is_constructed_;
        IsConstructedField(bool isconstructed):is_constructed_(isconstructed){}
    };
    template<>
    // performance test later
    struct IsConstructedField<false,false>{
        alignas(cache_line) bool is_constructed_;
        IsConstructedField(bool isconstructed):is_constructed_(isconstructed){}
    };
    template <typename T>
    using RawData = std::array<std::byte,sizeof(T)>;
    template <typename T, bool HasSeq>
    class Cell;
    template <typename T,bool HasSeq>
    requires IsTrivial<T>
    class Cell<T,HasSeq>:public SeqField<HasSeq>{
        private:
            T data_;
        public:
            Cell() : SeqField<HasSeq>(0){}
            Cell(size_t seq) : SeqField<HasSeq>(seq){}
            void construct(T val) noexcept{
                data_ = val;
            }
            T read() const noexcept{
                return data_;
            }
            T &get() noexcept{
                return data_;
            }
            void destroy() noexcept{}
    };
    template<typename T,bool HasSeq>
    class Cell : public SeqField<HasSeq>,public IsConstructedField<HasSeq,std::is_trivially_destructible_v<T>>{
        private:
        // test later
        alignas(alignof(T) > cache_line ? alignof(T) : cache_line) RawData<T> data_;
        public:
            Cell() : SeqField<HasSeq>(0),IsConstructedField<HasSeq,std::is_trivially_destructible_v<T>>(false){}
            Cell(size_t seq) : SeqField<HasSeq>(seq),IsConstructedField<HasSeq,std::is_trivially_destructible_v<T>>(false){}
            // make Cell<T>  trivially destructible
            ~Cell() noexcept
            requires std::is_trivially_destructible_v<T> {}

            ~Cell() noexcept{
                if constexpr(!std::is_trivially_destructible_v<T>){
                    if(this->is_constructed_){
                        destroy();
                    }
                }
            }
            template<typename ...Args>
            void construct(Args &&... args) noexcept(std::is_nothrow_constructible_v<T,Args &&...>)
            requires std::is_constructible_v<T,Args &&...>{
                new(&data_) T(std::forward<Args>(args)...);
                if constexpr(!std::is_trivially_destructible_v<T>){
                    this->is_constructed_ = true;
                }
            }
            void destroy() noexcept{
                if constexpr(!std::is_trivially_destructible_v<T>){
                    reinterpret_cast<T*>(&data_)->~T();
                    this->is_constructed_ = false;
                }
            }
            T &&read() noexcept{
                return reinterpret_cast<T &&>(data_);
            }
            T &get() noexcept{
                return reinterpret_cast<T &>(data_);
            }
    };
    template<typename T, size_t N, typename SizeConstraint>
    concept IsValidMPMCQueue = ValidSizeParameter<N,SizeConstraint> && FalseSharingSafe<Cell<T,true>>;
    
static inline bool cas_add(std::atomic<size_t> &seq,size_t val) noexcept{
    return seq.compare_exchange_weak(val,val+1,std::memory_order_relaxed,std::memory_order_relaxed);
}

// MPMC queue
    template<
        typename T,
        size_t N,
        bool Modulo = true,
        IsValidConstraint SizeConstraint = EnablePowerOfTwo,
        IsValidBufferType BufferType = UseHeapBuffer
    >
    requires IsValidMPMCQueue<T,N,SizeConstraint>
    class MPMCQueue{
        private:
            static constexpr bool UseStack = std::is_same_v<BufferType,UseStackBuffer>;
            using value_type = Cell<T,true>;
            using heap_buffer = HeapBuffer<value_type,SizeConstraint,Modulo,N>;
            using stack_buffer = StackBuffer<value_type,N,Modulo>;
            using allocator_type = std::allocator<value_type>;
            using buffer_type = std::conditional_t<UseStack,stack_buffer,heap_buffer>;
            // whether you need alignas buffer_
            alignas(cache_line) buffer_type buffer_;
            alignas(cache_line) const size_t buffer_size_;
            alignas(cache_line) std::atomic<size_t> head_;
            alignas(cache_line) std::atomic<size_t> tail_;
        public:
            explicit MPMCQueue(const size_t buffer_size = N, const allocator_type &allocator = allocator_type()) noexcept:
            buffer_(buffer_size,allocator),
            buffer_size_(buffer_size){
            // TODO:size validation

            for(size_t i = 0; i < buffer_size_; ++i){
                new(&buffer_[i]) value_type(i);
            }

        }
        ~MPMCQueue() noexcept{
            for(size_t i = 0; i < buffer_size_; ++i){
                buffer_[i].~Cell();
            }
        }
        MPMCQueue(const MPMCQueue&) = delete;
        MPMCQueue& operator=(const MPMCQueue&) = delete;
        MPMCQueue(MPMCQueue&& other) = delete;
        MPMCQueue& operator=(MPMCQueue&& other) = delete;
        template<typename ...Args>
        void emplace(Args &&... args) noexcept(std::is_nothrow_constructible_v<T,Args &&...>)
        requires std::is_constructible_v<T,Args &&...>{
            size_t pos = tail_.fetch_add(1,std::memory_order_relaxed);
            auto &cell = buffer_[pos];
            while(pos != cell.seq_.load(std::memory_order_acquire));
            cell.construct(std::forward<Args>(args)...);
            cell.seq_.store(pos + 1, std::memory_order_release);
        }
        void push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::is_copy_constructible_v<T>{
            emplace(value);
        }
        // use P to construct T
        template<typename P>
        void push(P &&value) noexcept(std::is_nothrow_constructible_v<T,P>)
        requires std::is_constructible_v<T,P>{
            emplace(std::forward<P>(value));
        }
        template<typename... Args>
        [[nodiscard]] bool try_emplace(Args &&... args) noexcept(std::is_nothrow_constructible_v<T,Args &&...>)
        requires std::is_constructible_v<T,Args &&...>{
            while(true){
                size_t pos = tail_.load(std::memory_order_relaxed);
                auto &cell = buffer_[pos];
                const size_t seq = cell.seq_.load(std::memory_order_acquire);
                const int64_t diff = seq - pos;
                if (diff == 0 && cas_add(tail_,pos)){
                    cell.construct(std::forward<Args>(args)...);
                    cell.seq_.store(pos + 1, std::memory_order_release);
                    return true;
                }
                else if (diff < 0){
                    return false;
                }
            }
        }
        [[nodiscard]] bool try_push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::is_copy_constructible_v<T>{
            return try_emplace(value);
        }
        template<typename P>
        [[nodiscard]] bool try_push(P &&value) noexcept(std::is_nothrow_constructible_v<T,P>)
        requires std::is_constructible_v<T,P>{  
            return try_emplace(std::forward<P>(value));
        }
        void pop(T &value) noexcept{
            const size_t pos = head_.fetch_add(1,std::memory_order_relaxed);
            auto &cell = buffer_[pos];
            while(pos + 1 != cell.seq_.load(std::memory_order_acquire));
            value = cell.read();
            cell.destroy();
            cell.seq_.store(pos + buffer_size_, std::memory_order_release);
        }
        [[nodiscard]] bool try_pop(T &value) noexcept{
            while(true){
                const size_t pos = head_.load(std::memory_order_relaxed);
                auto &cell = buffer_[pos];
                const size_t seq = cell.seq_.load(std::memory_order_acquire);
                const int64_t diff = seq - pos;
                if (diff == 1 && cas_add(head_,pos)){
                    value = cell.read();
                    cell.destroy();
                    cell.seq_.store(pos + buffer_size_, std::memory_order_release);
                    return true;
                }
                else if (diff < 1){
                    return false;   
                }
            }
        }
    };
    // SPMC queue
    
    template<
        typename T,
        size_t N,
        bool Modulo = true,
        IsValidConstraint SizeConstraint = EnablePowerOfTwo,
        IsValidBufferType BufferType = UseHeapBuffer
    >
    requires IsValidMPMCQueue<T,N,SizeConstraint>
    class SPMCQueue{
        private:
            static constexpr bool UseStack = std::is_same_v<BufferType,UseStackBuffer>;
            using value_type = Cell<T,true>;
            using heap_buffer = HeapBuffer<value_type,SizeConstraint,Modulo,N>;
            using stack_buffer = StackBuffer<value_type,N,Modulo>;
            using allocator_type = std::allocator<value_type>;
            using buffer_type = std::conditional_t<UseStack,stack_buffer,heap_buffer>;
            
            alignas(cache_line) buffer_type buffer_;
            alignas(cache_line) const size_t buffer_size_;
            alignas(cache_line) size_t write_idx_;
            
        public:
            struct Reader {
                operator bool() const { return queue_ != nullptr; }
                
                T* read() noexcept {
                    auto& cell = queue_->buffer_[next_idx_];
                    size_t cell_seq = cell.seq_.load(std::memory_order_acquire);
                    if (int64_t(cell_seq - next_idx_) < 0) return nullptr;
                    next_idx_ = cell_seq + 1;
                    return &cell.get();
                }
                SPMCQueue* queue_ = nullptr;
                size_t next_idx_;
            };
            
            explicit SPMCQueue(const size_t buffer_size = N, const allocator_type &allocator = allocator_type()) noexcept:
            buffer_(buffer_size,allocator),
            buffer_size_(buffer_size),
            write_idx_(0) {
                // 初始化所有单元格
                for(size_t i = 0; i < buffer_size_; ++i){
                    new(&buffer_[i]) value_type(i);
                }
            }
            
            ~SPMCQueue() noexcept {
                for(size_t i = 0; i < buffer_size_; ++i){
                    buffer_[i].~Cell();
                }
            }
            
            SPMCQueue(const SPMCQueue&) = delete;
            SPMCQueue& operator=(const SPMCQueue&) = delete;
            SPMCQueue(SPMCQueue&& other) = delete;
            SPMCQueue& operator=(SPMCQueue&& other) = delete;
            
            Reader getReader() noexcept {
                Reader reader;
                reader.queue_ = this;
                reader.next_idx_ = write_idx_ + 1;
                return reader;
            }
            
            template<typename ...Args>
            void emplace(Args &&... args) noexcept(std::is_nothrow_constructible_v<T,Args &&...>)
            requires std::is_constructible_v<T,Args &&...> {
                auto& cell = buffer_[++write_idx_];
                cell.construct(std::forward<Args>(args)...);
                cell.seq_.store(write_idx_, std::memory_order_release);
            }
            void push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires std::is_copy_constructible_v<T> {
                emplace(value);
            }
            
            template<typename P>
            void push(P &&value) noexcept(std::is_nothrow_constructible_v<T,P>)
            requires std::is_constructible_v<T,P> {
                emplace(std::forward<P>(value));
            }
    };
}
