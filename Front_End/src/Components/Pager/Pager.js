import React,{Component} from 'react'
import Uarr from '../asc.ico';
import './Pager.css'


const pages = (cur, end, setPage) => {
    let pages = [];
    if(end < 10){
        for (let i = 0; i < end; i++) {
            pages.push(
                <div key={i} onClick={() => setPage(i)}
                    style={{ cursor: 'pointer' }} className={(i === cur) ? 'curPage' : 'page'}>
                    {i + 1}
                </div>
            );
        }
    }
    else { 
        pages.push(
            <div key={0} onClick={() => setPage(0)}
                style={{ cursor: 'pointer' }} className={(cur === 0) ? 'curPage' : 'page'}>
                {1}
            </div>
        )
        if (cur > 3)
            pages.push(<div className="page" key={-1}>...</div>)  
        for (let i = Math.max(cur-2,1); i < Math.min(cur+3,Math.floor(end)); i++){
            pages.push(
            <div key={i} onClick={() => setPage(i)}
                style={{ cursor: 'pointer' }} className={(cur === i) ? 'curPage' : 'page'}>
                {i+1}
            </div>
            )
        }
        if(cur < Math.floor(end)-3)
            pages.push(<div className="page" key={-2}>...</div>)
        pages.push(
            <div key={Math.floor(end)} onClick={() => setPage(Math.floor(end))}
                style={{ cursor: 'pointer' }} className={(cur === Math.floor(end)) ? 'curPage' : 'page'}>
                {Math.floor(end)+1}
            </div>
        )
    }
    return pages;
}



class Pager extends Component{
    constructor(props){
        super(props)
        this.selPage = 0;
    }
    
    render(){
        return(
        <div className='pager'>
            <div style={{ display: 'inline', left: '0px', position: 'absolute' }}>{Math.min(this.props.totalObjs,(this.props.size * this.props.curPage + 1))}-
                {Math.min(this.props.size * (this.props.curPage + 1), this.props.totalObjs)} from {this.props.totalObjs}</div>
            {
                (this.props.curPage > 0) ?
                    <div onClick={() => this.props.setPage(this.props.curPage - 1)}
                        className='prev'>
                        <img src={Uarr} alt={'&#8592;'} style={{ transform: "rotate(270deg)" }} />
                    </div>
                    : <div style={{ display: 'inline' }} />
            }
            <div className='pages'>
                {pages(this.props.curPage, this.props.maxPage, this.props.setPage)}
            </div>
            {
                (this.props.curPage < this.props.maxPage - 1) ?
                    <div onClick={() => this.props.setPage(this.props.curPage + 1)} style={{ cursor: 'pointer' }}
                        className='next'>
                        <img src={Uarr} alt={'&#8594;'} style={{ transform: "rotate(90deg)" }} />
                    </div>
                    : <div />
            }
            <form onSubmit={(e) => {e.preventDefault(); this.props.setPage(this.selPage);}}
                style={{display:'inline-block'}}>
                Page: <input type='number' onChange={(e) => 
                    this.selPage = parseInt(e.target.value,10) -1} min='1'
                    max={Math.floor(this.props.maxPage+1)} value={this.props.curPage+1}/>
            </form>
        </div>
        )
    }
}
export default Pager