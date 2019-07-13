import React,{Component} from 'react';
import Axios from 'axios';
import '../../global.js';
import './Source.css';

class Source extends  Component{
    constructor(props) {
        super(props);
        this.state = {
            html:"<h3>Loading..</h3>",
            loaded: false
        }
    }

    componentDidMount(){
        this.getSourceCode();
        console.log(this.state);
    }

    getSourceCode(){
        Axios.get(global.address+"qsrc.html?id="+this.props.id+"&qt=id&match=Y&writable=1&a2=1&n=Source+Code+With+Identifier+Hyperlinks")
        .then((response) => {
            if(response.data.error)
                this.setState({
                    error:response.data.error
                })
            else{
                this.setState({
                    source: response.data.source,
                    html: response.data.html.file,
                    loaded: true
                })
            }
        })

       
    }
 
    returnHtml(obj){
        return{
            __html: obj
        };
    }

    contentClickHandler = (e) => {
        const targetLink = e.target.closest('a');
        if(!targetLink) return;
        e.preventDefault();
        
        console.log(targetLink.href); 
      };


    render(){
        console.log(this.state.html);
        return(
            <div className='source'>
                <div>
                    {(this.state.loaded)? 
                    <div onClick={this.contentClickHandler} 
                    dangerouslySetInnerHTML={this.returnHtml(this.state.html)}>              
                </div>
            :this.state.html}
                </div>
            </div>
        );
    }
}

export default Source;