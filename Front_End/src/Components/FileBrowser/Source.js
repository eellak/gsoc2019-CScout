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

    parseHtml(){
        return(
            <div>
                {this.state.html.map((obj,i) => 
                    (obj.type==="html")?
                    <div key={i} className="srcHtml" dangerouslySetInnerHTML={this.returnHtml(obj.html)} />
                    :<div key={i} className="srcLink" onClick={() => {
                            this.props.changeType(obj)
                        } }>{obj.name}<br/></div>
                    )}
            </div>
        )
    }

    returnHtml(obj){
        return{
            __html: obj
        };
    }

    render(){
        console.log(this.returnHtml);
        return(
            <div className='source' 
            >
                <div>
                    {(this.state.loaded)?this.parseHtml():this.state.html}
                </div>
            </div>
        );
    }
}

export default Source;