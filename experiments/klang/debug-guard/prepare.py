"""Apply the debug opt-out to the reviewed UE-additions snapshot."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
source = ROOT/'experiments/klang/ue-integration/klang.h'
text = source.read_text()
text = text.replace('#define HAS_KLANG 1', '''#define HAS_KLANG 1

// Studio wrappers define KLANG in both Release and Debug builds.
// Other hosts may opt in explicitly; use one setting throughout a module.
#ifndef HAS_KLANG_DEBUG
#if defined(KLANG) && KLANG
#define HAS_KLANG_DEBUG 1
#else
#define HAS_KLANG_DEBUG 0
#endif
#endif''', 1)
start = text.index('\tstruct Debug : Input {')
end = text.index('\n\t//inline static Debug::Buffer&', start)
block = text[start:end]

def guard(signature, disabled, occurrence=0):
    global block
    positions = []
    offset = 0
    while (offset := block.find(signature, offset)) >= 0:
        positions.append(offset)
        offset += len(signature)
    a = block.index('{', positions[occurrence])
    depth, b = 1, a+1
    # Only these existing function bodies are processed; braces in comments balance.
    while depth:
        depth += (block[b] == '{') - (block[b] == '}')
        b += 1
    body = block[a+1:b-1]
    indent = body[body.rfind('\n')+1:]
    body = body[:body.rfind('\n')+1]
    block = block[:a+1]+'\n#if HAS_KLANG_DEBUG'+body+'#else\n'+disabled+'\n#endif\n'+indent+block[b-1:]

guard('const float* get()', '\t\t\t\treturn nullptr;')
guard('Buffer& operator+=(const signal in)', '\t\t\t\treturn *this;')
guard('Buffer& operator+=(TYPE& in)', '\t\t\t\treturn *this;')
guard('signal& operator>>(signal& destination) const', '\t\t\t\tdestination = 0;\n\t\t\t\treturn destination;')
guard('operator const signal& () const', '\t\t\t\tstatic const signal silence = 0;\n\t\t\t\treturn silence;')
guard('inline static double profile(Func func, Args... args)', '\t\t\treturn 0;')
guard('inline static double profile(int times, Func func, Args... args)', '\t\t\treturn 0;')
guard('Session(float*, int size, Buffer::Content content)', '')
guard('bool hasAudio() const', '\t\t\t\treturn false;')
guard('const float* getAudio() const', '\t\t\t\treturn nullptr;')
guard('void print(const char* format, ...)', '')
guard('void printOnce(const char* format, ...)', '')
guard('bool hasText() const', '\t\t\treturn false;')
guard('int getText(char* buffer)', '\t\t\tif (buffer) buffer[0] = 0;\n\t\t\treturn 0;')
guard('void input() override', '')
guard('operator const signal& () const', '\t\t\treturn passthrough();', occurrence=1)
block = block.replace('\t\t\tusing buffer::operator++;', '''#if HAS_KLANG_DEBUG
\t\t\tusing buffer::operator++;
#else
\t\t\t// Keep direct cursor calls valid without advancing beyond storage.
\t\t\tsignal& operator++(int) { return *ptr; }
#endif''')
block = block.replace('using Input::input;', '''using Input::input;
#if !HAS_KLANG_DEBUG
\t\t// Keep inline debug taps transparent without touching shared Input::in.
\t\tstatic signal& passthrough() {
\t\t\tstatic thread_local signal value = 0;
\t\t\treturn value;
\t\t}
\t\tvoid input(const signal& value) override { passthrough() = value; }
\t\tvoid operator<<(const signal& value) override { passthrough() = value; }
#endif''')
text = text[:start]+block+text[end:]
line = next(line for line in text.splitlines() if line.startswith('#define PROFILE('))
text = text.replace(line, '#if HAS_KLANG_DEBUG\n'+line+'\n#else\n#define PROFILE(func, ...) ((void)0);\n#endif')
lines = text.splitlines()
result = []
for line in lines:
    if line.strip() == 'debug.buffer++;':
        result.extend(['#if HAS_KLANG_DEBUG', line, '#endif'])
    else:
        result.append(line)
(Path(__file__).parent/'klang.h').write_bytes(('\r\n'.join(result)+'\r\n').encode())
print('Prepared debug guard prototype; production unchanged.')
