import React, { Component } from 'react';
import '../../global.js';
import Axios from 'axios';
import './IdDependancies.css';
import Uarr from '../asc.ico'

class IdDependancies extends Component{
    constructor(props){
        super(props);
        this.state = {
            loaded:false,
            page:0,
            orderby: "",
            orderField: 0,
            rev: false    
        }
    }

    componentDidMount(){
        this.getFiles()
    }
    showPage(){
        var  toRender = [];
        var start = this.state.page * this.state.size;
        var i;
        for(i = 0; i < this.state.size; i++){
            if((start + i) >= this.state.show.length){
                break;
            }
            console.log(this.state.show[start+i])
            toRender.push(<tr key={i}>
                <td onDoubleClick={(e) => {
                        console.log(e.target.id)
                        this.props.changeType("filePage",e.target.id)
                    
                    }} 
                    id={this.state.show[start+i].id} style={{cursor:'pointer'}}>{this.state.show[start + i].name}</td>
                <td>{this.state.show[start + i].path}</td>
                {this.state.metric?<td>{this.state.show[start + i].metric}</td>:null}
                </tr>);
        }
        
        return toRender;
    }

    objectComp(a,b,e){
        if ( a[e] < b[e]){
            return -1;
        }
        if ( a[e] > b[e] ){
            return 1;
        }
        return 0;
    }

    orderTable(e){
        var files = this.state.show;           
        files.sort((a,b) => this.objectComp(a,b,e));
        this.setState({
            show: files
        })   
    }

    changeOrder(e){ 
        if(this.state.orderField !== e){    
            console.log(e)
            console.log(Object.keys(this.state.files[0])[e])
            this.setState({
                orderby:Object.keys(this.state.files[0])[e],
                orderField:e,
                rev:false
            })
            this.orderTable(Object.keys(this.state.files[0])[e]);
        }
        else{
            if(this.state.rev){
                this.setState({
                    rev:false,
                    orderField:0
                })
                
            }
            else
                this.setState({
                    rev: !this.state.rev,
                    show: this.state.show.reverse()
                });
        }
    }

    getFiles(){
        Axios.get(global.address + "xiquery.html?"+this.props.search)
        .then((response) => {
            if(response.data.error){
                this.setState({
                    error: response.data.error
                })
            } else
            {
                if(!response.data.files){
                    this.setState({
                        files: [],
                        timer: response.data.timer,
                        loaded:true,
                        show: [],
                        size: 20,
                        start: 0,
                        page:0
                    })
                } else 
                this.setState({
                    files: response.data.files.files,
                    timer: response.data.timer,
                    loaded:true,
                    show: response.data.files.files,
                    size: 20,
                    start: 0,
                    page:0
                })                   
                console.log(this.state)
            }
        });
    }

    render(){
        return(
            <div>
            {this.state.loaded?<table className='FileDepends'>
                <thead>
                    <tr>
                    <td onClick={() => {this.changeOrder(1)}}>
                                    Name
                                    {
                                        (this.state.orderField === 1)?
                                            <img src={Uarr} align="right" style={(this.state.rev)?
                                            {transform: "scaleY(-1)"}
                                            :{}
                                            }/>
                                        :""                                   
                                    }
                                </td>
                                <td onClick={() => {this.changeOrder(2)}}>
                                    Path
                                    {
                                        (this.state.orderField === 2)?
                                            <img src={Uarr} align="right" style={(this.state.rev)?
                                            {transform: "scaleY(-1)"}
                                            :{}
                                            }/>
                                        :""                                   
                                    }
                                </td>
                    </tr>
                </thead>
            <tbody>{this.showPage()}</tbody>
            </table>:<div>Loading...</div>}
        </div>
        )
    }
}

export default IdDependancies;
