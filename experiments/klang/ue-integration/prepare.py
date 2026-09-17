from pathlib import Path
import zipfile
p=Path('experiments/klang/ue-integration/klang.h')
with zipfile.ZipFile('experiments/klang/osm-improved.zip') as archive:
 s=archive.read('include/klang.h').decode().replace('\r\n','\n')
s=s.replace('\n\tstruct Graph;','\n'+Path('experiments/klang/ue-integration/interleaved.inc').read_text()+'\n\tstruct Graph;',1)
s=s.replace('\tvoid Controls::set(Preset& preset)', '\tinline void Controls::set(Preset& preset)')
s=s.replace('level += abs(samples[s]);','level += FABS(samples[s]);')
s=s.replace('void set(float r) { this->r = r; }','void set(param r) override { this->r = r; }')
a=s.index('\tstruct GraphPtr {');b=s.index('\n\t};',a)+5
s=s[:a]+s[a:b].replace('check()', 'init()')+s[b:]
for call in ['control(index, value)','preset(index)','midi(status, byte1, byte2)']:
 import re
 s=re.sub(r'(?<![\w>.:])'+re.escape(call), 'this->'+call,s)
s=s.replace('0b111111111111111U)', '0b111111111111111UL)')
a=s.index('\t\t\t/// Triangle wave oscillator (aliased)')
s=s[:a]+'''\t\t\t/// Aliased 0-to-1 ramp; frequency in Hz, absolute phase in radians, like Basic::Saw.
\t\t\tstruct Phasor : Saw {
\t\t\t\tvoid process() override {
\t\t\t\t\tSaw::process();
\t\t\t\t\tout = out * 0.5f + 0.5f;
\t\t\t\t}
\t\t\t};

'''+s[a:]
a=s.index('\t\t\t/// White noise generator (optimised)')
s=s[:a]+'''\t\t\t/// The basic aliased ramp is also available through klang::optimised.
\t\t\tstruct Phasor : Basic::Phasor {};

'''+s[a:]
a=s.index('\t\t/// Stereo audio object adapter')
s=s[:a]+'''\t\tnamespace interleaved {
\t\t\tusing buffer = klang::interleaved::buffer<2>;
\t\t}

'''+s[a:]
a=s.index('\t\t/// Stereo effect mini-plugin');b=s.index('\n\t\tstruct Sound : Effect',a)
block=s[a:b];pos=block.rindex('\n\t\t};')
block=block[:pos]+'''
\t\t\t/// Process interleaved host frames; inherited by the current Stereo::Sound.
\t\t\tvirtual void process(interleaved::buffer buffer) {
\t\t\t\tthis->prepare();
\t\t\t\twhile (!buffer.finished()) {
\t\t\t\t\tinput(buffer);
\t\t\t\t\tthis->process();
\t\t\t\t\tbuffer++ = out;
\t\t\t\t\tdebug.buffer++;
\t\t\t\t}
\t\t\t}
'''+block[pos:]
s=s[:a]+block+s[b:]
p.write_bytes(s.replace('\n','\r\n').encode())
print('Prepared independent UE additions without changing production.')
