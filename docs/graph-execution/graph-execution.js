(() => {
  const stage = document.getElementById('stage');
  const readout = document.getElementById('readout');
  const playBtn = document.getElementById('playBtn');
  const branchBtn = document.getElementById('branchBtn');
  const payload = document.getElementById('movingPayload');
  const reduced = matchMedia('(prefers-reduced-motion: reduce)');
  const steps = [
    {name:'Input', active:'input', path:0, point:[252,277], text:'Runtime activates Input · 1 / 5'},
    {name:'Processor', active:'process', path:0, point:[341,306], text:'Payload enters Processor mailbox · 2 / 5'},
    {name:'Condition', active:'gate', path:1, point:[505,252], text:'Condition evaluates message · 3 / 5'},
    {name:'Output', active:null, path:2, point:[603,280], text:'Selected output carries Payload · 4 / 5'},
    {name:'Destination', active:null, path:2, point:[665,264], text:'Destination mailbox receives Payload · 5 / 5'}
  ];
  let index = -1, branch = 0, timer = 0, anim = 0;
  const names = ['input','process','gate','out1','out2'];
  const comps = Object.fromEntries(names.map(name => [name, document.querySelector(`[data-component="${name}"]`)]));
  const statuses = Object.fromEntries(names.map(name => [name, document.querySelector(`[data-status="${name}"]`)]));
  const flows = [...document.querySelectorAll('[data-flow]')];
  const clamp = (v,a,b) => Math.max(a,Math.min(b,v));
  function setState(name,state) {
    const node=comps[name]; if(!node)return;
    node.classList.remove('running','completed','dim');
    if(state==='running')node.classList.add('running');
    if(state==='completed')node.classList.add('completed');
    if(state==='dim')node.classList.add('dim');
    if(statuses[name])statuses[name].textContent=state.toUpperCase();
  }
  function display() {
    stage.classList.remove('motion');
    flows.forEach(el=>el.style.opacity='0');
    payload.style.opacity='0';
    names.forEach(n=>setState(n,'idle'));
    comps.out1.classList.add('dim'); comps.out2.classList.add('dim');
    document.querySelector('.gate .output.selected').classList.remove('selected');
    document.querySelectorAll('.gate .output')[branch].classList.add('selected');
    if(index<0){readout.textContent='Ready · 0 / 5';playBtn.textContent='Run';return;}
    const step=steps[index];
    if(step.active)setState(step.active,index===0?'running':'completed');
    if(index>=1)setState('input','completed');
    if(index>=2)setState('process','completed');
    if(index>=3)setState('gate','completed');
    const selected=branch===0?'out1':'out2', other=branch===0?'out2':'out1';
    comps[selected].classList.remove('dim'); comps[other].classList.add('dim');
    if(index===4)setState(selected,'completed');
    readout.textContent=step.text.replace('Output',branch===0?'Output 1':'Output 2');
    if(index===1||index===2){flows[0].style.opacity='1';}
    if(index>=3){flows[branch===0?2:3].style.opacity='1';}
    const pt=step.point;
    payload.setAttribute('transform',`translate(${pt[0]} ${pt[1]})`);
    payload.style.opacity='1';
    stage.classList.add('motion');
    if(index===0)playBtn.textContent='Pause';
    else playBtn.textContent='Run';
  }
  function next() { index = index >= steps.length-1 ? -1 : index+1; display(); }
  function run() {
    if(timer){clearInterval(timer);timer=0;playBtn.textContent='Run';return;}
    if(index>=steps.length-1)index=-1;
    playBtn.textContent='Pause';
    timer=setInterval(()=>{if(index>=steps.length-1){clearInterval(timer);timer=0;playBtn.textContent='Run';return;} index++; display();}, reduced.matches?1100:1550);
    if(index<0){index=0;display();}
  }
  document.getElementById('stepBtn').addEventListener('click',()=>{if(timer){clearInterval(timer);timer=0;}next();});
  playBtn.addEventListener('click',run);
  document.getElementById('resetBtn').addEventListener('click',()=>{if(timer)clearInterval(timer);timer=0;index=-1;display();});
  branchBtn.addEventListener('click',()=>{branch=1-branch;branchBtn.textContent=`Branch: output${branch+1}`;branchBtn.setAttribute('aria-label',`Selected condition output${branch+1}`);display();});
  display();
})();
