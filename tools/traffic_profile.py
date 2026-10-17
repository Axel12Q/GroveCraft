"""Owned traffic policy; accepts the user's game data without bundling it."""
import re

LIMIT=23  # CPopulation::m_CarGroups, verified in the original loader/table.

def vehicles(text):
    result={}
    for line in text.splitlines():
        # GTA accepts whitespace too (the shipped Wayfarer row lacks a comma).
        a=re.split(r'[,\s]+',line.split('#',1)[0].strip())
        if len(a)>=9 and a[0].isdigit() and a[3] in ('car','bike','bmx','quad','mtruck','trailer','boat','train','heli','plane'):
            result[a[1]]=dict(id=int(a[0]),type=a[3],category=a[7])
    return result

def groups(text):
    result=[]
    for line in text.splitlines():
        body,_,comment=line.partition('#')
        names=re.findall(r'[a-zA-Z0-9_]+',body)
        if names:result.append((names,comment.strip()))
    return result

def build(original,ide,profile):
    assert profile['version']==1,'Unknown traffic policy'
    catalogue=vehicles(ide);before=groups(original);out=[];changes=[];index=0
    policy=profile['groups'];seen=set()
    for line in original.splitlines(keepends=True):
        body,sep,comment=line.partition('#');names=re.findall(r'[a-zA-Z0-9_]+',body)
        if not names:out.append(line);continue
        key=comment.strip();entry=policy.get(key)
        if entry is not None:
            assert key not in seen and index==entry['index'],'Unexpected population group order'
            seen.add(key);add=entry['add']
            assert len(set(add))==len(add) and all(n not in names for n in add),'Duplicate traffic entry'
            assert len(names)+len(add)<=LIMIT,'Traffic group exceeds native capacity'
            for name in add:
                assert name in catalogue and catalogue[name]['type']=='car' and catalogue[name]['category']!='ignore','Unsupported traffic vehicle: '+name
            newline='\r\n' if line.endswith('\r\n') else '\n' if line.endswith('\n') else ''
            out.append(', '.join(names+add)+'\t# '+key+newline)
            changes.append(dict(index=index,group=key,before=names,after=names+add,added=add,
                added_ids=[catalogue[n]['id'] for n in add]))
        else:out.append(line)
        index+=1
    assert seen==set(policy),'Missing requested population groups'
    result=''.join(out);after=groups(result)
    assert len(before)==len(after),'Population group count changed'
    changed={c['index'] for c in changes}
    assert all(a==b for i,(a,b) in enumerate(zip(before,after)) if i not in changed),'Unrelated group changed'
    return result,changes
