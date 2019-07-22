import React from 'react'
import Uarr from '../../asc.ico';
import './Pager.css'


const pages = (cur, end, setPage) => {
    let pages = []
    for (let i = 0; i < end; i++) {
        pages.push(
            <div key={i} onClick={() => setPage(i)}
                style={{ cursor: 'pointer' }} className={(i === cur) ? 'curPage' : 'page'}>
                {i + 1}
            </div>
        );
    }
    return pages;
}

const Pager = props =>
    <div className='pager'>
        <div style={{ display: 'inline', left: '0px', position: 'absolute' }}>{props.size * props.curPage + 1}-
            {Math.min(props.size * (props.curPage + 1), props.totalObjs)} files from {props.totalObjs}</div>
        {
            (props.curPage > 0) ?
                <div onClick={() => props.setPage(props.curPage - 1)}
                    className='prev'>
                    <img src={Uarr} alt={'&#8592;'} style={{ transform: "rotate(270deg)" }} />
                </div>
                : <div style={{ display: 'none' }} />
        }
        <div className='pages'>
            {pages(props.curPage, props.maxPage, props.setPage)}
        </div>
        {
            (props.curPage < props.maxPage - 1) ?
                <div onClick={() => props.setPage(props.curPage + 1)} style={{ cursor: 'pointer' }}
                    className='next'>
                    <img src={Uarr} alt={'&#8594;'} style={{ transform: "rotate(90deg)" }} />
                </div>
                : <div />
        }
    </div>

export default Pager